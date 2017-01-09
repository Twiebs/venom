
static Engine g_engine;
thread_local U32 g_threadID;


static inline void ExecuteTask(Worker *worker, Task *task) {
  switch (task->type) {
    case TaskType_LoadModel: {
      CreateModelAssetFromFile(task->slotID);
    } break;
  }
}

static inline void FinalizeTask(Worker *worker, Task *task) {
  switch (task->type) {
  case TaskType_LoadModel: {
    CreateOpenGLResourcesForModelAsset(task->slotID);
  } break;
  }
}

static inline void FinalizeAllTasks(Engine *engine) {
  for (size_t i = 0; i < engine->tasksToFinalize.count; i++) {
    FinalizeTask(&engine->workers[0], &engine->tasksToFinalize[i]);
  }

  engine->tasksToFinalize.count = 0;
}

static void WorkerThreadProc(Worker *worker) {
  g_threadID = worker->workerID;
  LogDebug("Worker %u has started", worker->workerID);
  Engine *engine = &g_engine;
  while (engine->isEngineRunning) {
    engine->workLock.lock();
    if (engine->tasksToExecute.count > 0) {
      Task task = engine->tasksToExecute[engine->tasksToExecute.count - 1];
      engine->tasksToExecute.count -= 1;
      engine->workLock.unlock();
      ExecuteTask(worker, &task);
      engine->workLock.lock();
      engine->tasksToFinalize.PushBack(task);
      engine->workLock.unlock();
    } else {
      engine->workLock.unlock();
    }
  }

  LogDebug("Worker %u has exited", worker->workerID);
}

static inline Engine *GetEngine() {
  return &g_engine;
}

static inline U32 GetThreadID() {
  return g_threadID;
}

static const size_t WORKER_STACK_MEMORY_SIZE = 1024 * 1024 * 2;

void ScheduleTask(Task task) {
  auto engine = GetEngine();
  engine->workLock.lock();
  engine->tasksToExecute.PushBack(task);
  engine->workLock.unlock();
}

void InitalizeEngine() {
  Engine *engine = &g_engine;
  size_t workerCount = std::thread::hardware_concurrency();
  size_t requiredMemory = Align8(workerCount * sizeof(Worker));
  requiredMemory += Align8(WORKER_STACK_MEMORY_SIZE * workerCount);

  U8 *memory = (U8 *)MemoryAllocate(requiredMemory);
  memset(memory, 0x00, requiredMemory);
  engine->workers = (Worker *)memory;
  engine->workerCount = workerCount;
  engine->isEngineRunning = true;
  U8 *currentStackMemoryPtr = memory + Align8(workerCount * sizeof(Worker));
  g_threadID = 0;
  for (size_t i = 0; i < workerCount; i++) {
    Worker *worker = &engine->workers[i];
    worker->workerID = i;
    worker->stackMemory.memory = currentStackMemoryPtr;
    worker->stackMemory.size = WORKER_STACK_MEMORY_SIZE;
    currentStackMemoryPtr += WORKER_STACK_MEMORY_SIZE;
    worker->thread = std::thread(WorkerThreadProc, worker);
  }
}