// ===================================================================
//	Copyright (C) 2013 Tim Leonard
// ===================================================================
#include "Game\Scene\Voxels\ChunkLoader.h"

#include "Game\Scene\Voxels\ChunkManager.h"

#include "Engine\Engine\FrameTime.h"

#include "Generic\Threads\Thread.h"
#include "Generic\Threads\Mutex.h"
#include "Generic\Threads\MutexLock.h"
#include "Generic\Threads\Semaphore.h"

ChunkLoader::ChunkLoader(ChunkManager* manager, const ChunkManagerConfig& config)
	: m_config(config)
	, m_manager(manager)
	, m_running(true)
	, m_paused(false)
	, m_refresh_queue(false)
	, m_generator(manager, config)
{
	// Create work semaphore.
	m_semaphore = Semaphore::Create();
	DBG_ASSERT(m_semaphore != NULL);
	
	// Create pause semaphore.
	m_pause_semaphore = Semaphore::Create();
	DBG_ASSERT(m_pause_semaphore != NULL);

	// Create produced chunks mutex.
	m_loaded_chunks_mutex = Mutex::Create();
	DBG_ASSERT(m_loaded_chunks_mutex != NULL);

	// Create load/save thread.
	m_thread = Thread::Create(Static_Chunk_Load_Thread, this);
	DBG_ASSERT(m_thread != NULL);
	
	// Boot up threads.
	m_thread->Set_Affinity(1 << (1 % m_thread->Get_Core_Count()));
	m_thread->Set_Priority(ThreadPriority::Low);
	m_thread->Start();
}

ChunkLoader::~ChunkLoader()
{
	// Wait for threads to shut down.
	m_running = false;
	while (m_thread->Is_Running())
	{
		Thread::Get_Current()->Sleep(0.01f);
	}

	// Delete thread stuff.
	SAFE_DELETE(m_thread);
	SAFE_DELETE(m_semaphore);
	SAFE_DELETE(m_pause_semaphore);
	SAFE_DELETE(m_loaded_chunks_mutex);
}

Mutex* ChunkLoader::Get_Mutex()
{
	return m_loaded_chunks_mutex;
}

void ChunkLoader::Pause()
{
	DBG_ASSERT(!m_paused);

	m_paused = true;
	m_semaphore->Signal();
}

void ChunkLoader::Resume()
{
	m_paused = false;
	m_pause_semaphore->Signal();
}

void ChunkLoader::Refresh()
{
	m_refresh_queue = true;
	m_semaphore->Signal();
}

void ChunkLoader::Refresh_Queue()
{
	IntVector3 camera_chunk = m_manager->Get_Last_Camera_Chunk_Position();

	// Clear current chunk list.
	m_chunks.Clear();

	// Look for all chunks in our load distance.
	for (int x = camera_chunk.X - m_config.load_distance.X; x < camera_chunk.X + m_config.load_distance.X; x++)
	{
		for (int y = camera_chunk.Y - m_config.load_distance.Y; y < camera_chunk.Y + m_config.load_distance.Y; y++)
		{
			for (int z = camera_chunk.Z - m_config.load_distance.Z; z < camera_chunk.Z + m_config.load_distance.Z; z++)
			{
				IntVector3 chunk_position = IntVector3(x, y, z);
				Chunk*	   chunk		  = m_manager->Get_Chunk(chunk_position); 

				bool produced = false;
				
				{
					MutexLock lock(m_loaded_chunks_mutex);
					produced = m_loaded_chunk_positions.Contains(chunk_position);
				}

				// No chunk loaded? Time to load!
				if (chunk == NULL && produced == false)
				{
					m_chunks.Add(chunk_position);
				}
			}
		}
	}
}

void ChunkLoader::Static_Chunk_Load_Thread(Thread* thread, void* ptr)
{
	ChunkLoader* loader = reinterpret_cast<ChunkLoader*>(ptr);
	loader->Chunk_Load_Thread(thread);
}

void ChunkLoader::Chunk_Load_Thread(Thread* thread)
{
	int yield_counter = 0;

	while (m_running)
	{
		// Wait for work!
		m_semaphore->Wait();

		// Are we paused? If so wait for resume semaphore.
		if (m_paused == true)
		{
			m_pause_semaphore->Wait();
		}

		while (m_chunks.Size() > 0 ||
			   m_refresh_queue == true)
		{
			// Do we need to refresh our load list?
			if (m_refresh_queue == true)
			{
				Refresh_Queue();
				m_refresh_queue = false;
			}

			// Get frustum			
			Frustum frustum	= Renderer::Get()->Get_Frustum();

			// Do we need to update our queue?
			IntVector3 closest_chunk = IntVector3(0, 0, 0);
			bool found_chunk = false;

			// Find closest chunk to camera, cos thats what we will load!		
			const LinkedList<IntVector3>::Node* closest_chunk_node		= NULL;
			float								closest_distance		= 0.0f;
		
			IntVector3							camera_position			= m_manager->Get_Last_Camera_Chunk_Position();
		
			// Find closest chunk to load.
			for (LinkedList<IntVector3>::Iterator iter = m_chunks.Begin(); iter != m_chunks.End(); iter++)
			{
				IntVector3 chunk = *iter;

				// We bias height, as its better to load things on the same height as us than not. Keep smooooth scrolling :)
				IntVector3 vecDistance = (chunk - camera_position);
				vecDistance.Y *= CHUNK_LOAD_Y_BIAS;
				float distance = vecDistance.Length_Squared();

				AABB chunk_aabb = m_manager->Calculate_Chunk_AABB(chunk);

				// If chunk is in frustum, then bias it so its loaded earlier.
				//if (frustum.Intersects(chunk_aabb) != Frustum::IntersectionResult::Outside)
				//{
				//	distance /= 100.0f;
				//}

				if (found_chunk == false || distance < closest_distance)
				{
					closest_chunk = chunk;
					closest_distance = distance;
					closest_chunk_node = iter.Get_Node();
					found_chunk = true;
				}
			}
			
			// Start generating!
			if (found_chunk == true)
			{
				m_chunks.Remove(closest_chunk_node);

				// Wait until we have some memory for this chunk (we may need to wait for another
				// chunk to be unloaded).
				void* mem = NULL;
				while (true)
				{
					mem = m_manager->Get_Chunk_Memory_Pool().Allocate();
					if (mem != NULL)
					{
						break;
					}

					m_thread->Sleep(0.01f);
				}
					
				Chunk* chunk = new(mem) Chunk  (m_manager, closest_chunk.X, closest_chunk.Y, closest_chunk.Z, 
												m_config.chunk_size.X, m_config.chunk_size.Y, m_config.chunk_size.Z,
												m_config.voxel_size.X, m_config.voxel_size.Y, m_config.voxel_size.Z);

				chunk->Set_Status(ChunkStatus::Loading);
		
				// Load actual chunk.
				Load_Chunk(chunk);
				
				// Loaded!
				chunk->Set_Status(ChunkStatus::Loaded);
				chunk->Recalculate_State();

				// Use consumer.
				{
					MutexLock lock(m_loaded_chunks_mutex);
					m_loaded_chunks.Add(chunk);
					m_loaded_chunk_positions.Add(closest_chunk);
				}
			}
			
			// Yield the thread for a bit or we cause all kinds of hell.
			//yield_counter++;
			//if ((yield_counter % 5) == 0)
			//{
			//	m_thread->Sleep(0.005f);
			//}
		}
	}
}

void ChunkLoader::Load_Chunk(Chunk* chunk)
{
	// TODO: Load from file before generate!
	m_generator.Generate(chunk);
}

Chunk* ChunkLoader::Consume_Chunk()
{
	if (m_loaded_chunks.Size() > 0)
	{
		Chunk* c = m_loaded_chunks.Pop_First();
		m_loaded_chunk_positions.Pop_First();

		return c;
	}

	return NULL;
}