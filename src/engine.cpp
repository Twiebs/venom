
static Engine g_engine;
thread_local U32 g_threadID;


static inline B8 ExecuteTask(Worker *worker, Task task) {
  switch (task.type) {
    case TaskType_LoadModel: {
      return CreateModelAssetFromFile(task.slotID);
    } break;
  }
}

static inline void FinalizeTask(Worker *worker, Task task) {
  switch (task.type) {
  case TaskType_LoadModel: {
    CreateOpenGLResourcesForModelAsset(task.slotID);
  } break;
  }
}

static void WorkerThreadProc(Worker *worker) {
#ifndef VENOM_SINGLE_THREADED
  g_threadID = worker->workerID;
  LogDebug("Worker %u has started", worker->workerID);
  Engine *engine = &g_engine;
  while (engine->isEngineRunning) {
    engine->workLock.lock();
    if (engine->tasksToExecute.count > 0) {
      Task task = engine->tasksToExecute[engine->tasksToExecute.count - 1];
      engine->tasksToExecute.count -= 1;
      engine->workLock.unlock();
      ExecuteTask(worker, task);
      engine->workLock.lock();
      engine->tasksToFinalize.PushBack(task);
      engine->workLock.unlock();
    } else {
      engine->workLock.unlock();
    }
  }

  LogDebug("Worker %u has exited", worker->workerID);
#endif//VENOM_SINGLE_THREADED
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
  BeginProfileEntry("Initalize Engine");
  Engine *engine = &g_engine;

#ifdef VENOM_SINGLE_THREADED
  size_t workerCount = 1;
#else VENOM_SINGLE_THREADED
  size_t workerCount = std::thread::hardware_concurrency();
#endif//VENOM_SINGLE_THREADED

  size_t requiredMemory = Align8(workerCount * sizeof(Worker));
  requiredMemory += Align8(WORKER_STACK_MEMORY_SIZE * workerCount);
  U8 *memory = (U8 *)MemoryAllocate(requiredMemory);
  memset(memory, 0x00, requiredMemory);
  engine->workers = (Worker *)memory;
  engine->workerCount = workerCount;
  engine->isRunning = true;
  U8 *currentStackMemoryPtr = memory + Align8(workerCount * sizeof(Worker));
  g_threadID = 0; //Set the main thread's id to 0
  for (size_t i = 0; i < workerCount; i++) {
    Worker *worker = &engine->workers[i];
    worker->workerID = i;
    worker->stackMemory.memory = currentStackMemoryPtr;
    worker->stackMemory.size = WORKER_STACK_MEMORY_SIZE;
    currentStackMemoryPtr += WORKER_STACK_MEMORY_SIZE;
  }

  for (size_t i = 1; i < workerCount; i++) {
    Worker *worker = &engine->workers[i];
    worker->thread = std::thread(WorkerThreadProc, worker);
  }

  { //Initalize physics simulation paramaters
    PhysicsSimulation *sim = &engine->physicsSimulation;
    sim->gravityAcceleration = V3(0.0f, -9.81f, 0.0f);
  }

  InitalizeTerrain(&engine->terrain, 5, 8, 32);


#ifndef VENOM_RELEASE
  OpenGLEnableDebug(&engine->debugLog);
#endif//VENOM_RELEASE
  

  EndProfileEntry();
}

void FinalizeEngineTasks(Engine *engine) {



#ifdef VENOM_SINGLE_THREADED
  for (size_t i = 0; i < engine->tasksToExecute.count; i++) {
    if (ExecuteTask(&engine->workers[0], engine->tasksToExecute[i])) {
      engine->tasksToFinalize.PushBack(engine->tasksToExecute[i]);
    }
  }
  engine->tasksToExecute.count = 0;
#endif//VENOM_SINGLE_THREADED

  for (size_t i = 0; i < engine->tasksToFinalize.count; i++)
    FinalizeTask(&engine->workers[0], engine->tasksToFinalize[i]);
  engine->tasksToFinalize.count = 0;
}

void UpdateAnimationStates(EntityContainer *entityContainer, F32 deltaTime) {
  BeginTimedBlock("Update Animation States");
  EntityBlock *block = entityContainer->firstAvaibleBlock;
  for (U64 i = 0; i < entityContainer->capacityPerBlock; i++) {
    if (block->flags[i] & EntityFlag_PRESENT) {
      Entity* entity = &block->entities[i];
      ModelAsset *model = GetModelAsset(entity->modelID);
      if (model == nullptr) continue;
      if (model->jointCount > 0) {
        UpdateAnimationState(&entity->animation_state, model, deltaTime);
      }
    }
  }
  EndTimedBlock("Update Animation States");
}