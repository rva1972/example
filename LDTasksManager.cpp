#include "LevelLogic.h"
#include "LDTasksManager.h"

// FLD
FLDTargetActor::FLDTargetActor() :
  TeamId(0)
, MarkedActor(nullptr)
, SocketName(FName(TEXT("")))
, TagName(FName(TEXT("")))
{}

// FLDTask
FLDTask::FLDTask() :
  Id(0)
, State(ETaskState::NonActive)
, IsPrimary(true)
{}

// ALDTasksManager
ALDTasksManager::ALDTasksManager(const FObjectInitializer& PCIP) : Super(PCIP)
, ShowTaskId(-1)
, IsUpdate(false)
, IsReserveCall(true)
{
	EmptyReturn.Empty();

	bReplicates = true;
	bAlwaysRelevant = true;
}

void ALDTasksManager::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority)
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ALDTasksManager::GarbageCollector, 3.0f, true);
}

bool ALDTasksManager::CreateTask(int32 Id, FText Name, FText Description, ETaskState State, bool IsPrimary)
{
	auto Existing = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (Existing != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::CreateTask. Task with ID = %i already exists"), Id);
		return false;
	}

	FLDTask Task;

	Task.Id = Id;
	Task.Name = Name;
	Task.Description = Description;
	Task.State = State;
	Task.IsPrimary = IsPrimary;
	Task.MarkedActors.Empty();

	m_Tasks.Tasks.Add(Task);
	
	// For client on Listen Server side
	OnRep_OnServer();

	IsUpdate = true;

	return true;
}

bool ALDTasksManager::RemoveTask(int32 Id)
{
	auto Existing = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (Existing == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::RemoveTask. Task with ID = %i is not found"), Id);
		return false;
	}

	FLDTask Task = *Existing;

	m_Tasks.Tasks.RemoveAll([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	UE_LOG(LogTemp, Display, TEXT("ALDTasksManager::RemoveTask. Task with ID = %i was removed"), Id);

	// For client on Listen Server side
	OnRep_OnServer();

	IsUpdate = true;

	return true;
}

void ALDTasksManager::SetState(int32 Id, ETaskState State)
{
	auto Existing = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (Existing == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::SetState. Task with ID = %i is not found"), Id);
		return;
	}

	FLDTask& Task = *Existing;

	if (Task.State != State)
		Task.State = State;

	IsUpdate = true;

	// For client on Listen Server side
	OnRep_OnServer();
}

void ALDTasksManager::CheckState(int32 Id, ETaskState State, bool NotEqual, EResult& Result)
{
	if (CompareState(Id, State, NotEqual))
		Result = EResult::ExecTrue;
	else
		Result = EResult::ExecFalse;
}
 
bool ALDTasksManager::CompareState(int32 Id, ETaskState State, bool NotEqual)
{
	auto Existing = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (Existing == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::CompareState. Task with ID = %i is not found"), Id);
		return false;
	}

	const FLDTask& Task = *Existing;

	if (NotEqual)
		return Task.State != State;

	// For client on Listen Server side
	OnRep_OnServer();

	IsUpdate = true;

	return Task.State == State;
}

void ALDTasksManager::UpdateTask(int32 Id, FText Name, FText Description, bool IsPrimary)
{
	auto Existing = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (Existing == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::UpdateTask. Task with ID = %i is not found"), Id);
		return;
	}

	FLDTask& Task = *Existing;

	Task.Name = Name;
	Task.Description = Description;
	Task.IsPrimary = IsPrimary;

	IsUpdate = true;

	// For client on Listen Server side
	OnRep_OnServer();
}

const TArray<FLDTask>& ALDTasksManager::GetTasks() const
{
	return m_Tasks.Tasks;
}

const TArray<FLDTargetActor>& ALDTasksManager::GetMarkedActors(int32 Id) const
{
	auto ExistingTask = m_Tasks.Tasks.FindByPredicate([Id](const FLDTask& Task) {
		return Task.Id == Id;
	});

	if (ExistingTask == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::GetMarkedActors. Task with ID = %i is not found"), Id);
		return EmptyReturn;
	}

	return ExistingTask->MarkedActors;
}

void ALDTasksManager::AddTaskMarkerToActor(int32 TaskId, int32 TeamId, AActor* Target, FName SocketName, FName TagName)
{
	auto ExistingTask = m_Tasks.Tasks.FindByPredicate([TaskId](const FLDTask& Task) {
		return Task.Id == TaskId;
	});

	if (ExistingTask == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::AddMissionObjectiveToActor. Task with ID = %i is not found"), TaskId);
		return;
	}

	auto ExistingActor = ExistingTask->MarkedActors.FindByPredicate([Target, TeamId, SocketName, TagName](const FLDTargetActor& TargetActor) {
		return TargetActor.MarkedActor == Target && TargetActor.SocketName == SocketName && TargetActor.TagName == TagName && TargetActor.TeamId == TeamId;
	});

	if (ExistingActor != nullptr)
		return;

	FLDTargetActor NewTargetActor;
	NewTargetActor.MarkedActor = Target;
	NewTargetActor.TeamId = TeamId;
	NewTargetActor.SocketName = SocketName;
	NewTargetActor.TagName = TagName;
	
	ExistingTask->MarkedActors.Add(NewTargetActor);

	IsUpdate = true;

	// For client on Listen Server side
	OnRep_OnServer();
}

void ALDTasksManager::AddTaskMarkerToListActors(int32 TaskId, int32 TeamId, TArray<AActor*> Targets)
{
	for (auto& Target : Targets)
		AddTaskMarkerToActor(TaskId, TeamId, Target);
}

void ALDTasksManager::RemoveTaskMarkerFromActor(int32 TaskId, int32 TeamId, AActor* Target, FName SocketName, FName TagName)
{
	auto ExistingTask = m_Tasks.Tasks.FindByPredicate([TaskId](const FLDTask& Task) {
		return Task.Id == TaskId;
	});

	if (ExistingTask == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::RemoveMissionObjectiveFromActor. Task with ID = %i is not found"), TaskId);
		return;
	}

	ExistingTask->MarkedActors.RemoveAll([Target, SocketName, TagName, TeamId](const FLDTargetActor& TargetActor) {
		return TargetActor.MarkedActor == Target && TargetActor.SocketName == SocketName && TargetActor.TagName == TagName && TargetActor.TeamId == TeamId;
	});

	IsUpdate = true;

	// For client on Listen Server side
	OnRep_OnServer();
}

void ALDTasksManager::RemoveAllTaskMarkersForActor(AActor* Target, int32 TeamId)
{
	for (auto& Task : m_Tasks.Tasks)
		RemoveTaskMarkerFromActor(Task.Id, TeamId, Target);
}

void ALDTasksManager::RemoveAllTaskMarkers(int32 TaskId)
{
	auto ExistingTask = m_Tasks.Tasks.FindByPredicate([TaskId](const FLDTask& Task) {
		return Task.Id == TaskId;
	});

	if (ExistingTask == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::RemoveAllMissionObjectives. Task with ID = %i is not found"), TaskId);
		return;
	}

	ExistingTask->MarkedActors.Empty();

	IsUpdate = true;

	// For client on Listen Server side
	OnRep_OnServer();
}

void ALDTasksManager::OnRep_ChangeTask()
{
	if (OnUpdateTask.IsBound())
		OnUpdateTask.Broadcast();
}

ALDTasksManager* ALDTasksManager::GetTasksManager()
{
	if (GWorld == nullptr)
		return nullptr;

	TActorIterator<ALDTasksManager> It(GWorld);
	if (!It)
		return nullptr;

	return *It;
}

void ALDTasksManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALDTasksManager, ShowTaskId);
	DOREPLIFETIME(ALDTasksManager, m_Tasks);
}

void ALDTasksManager::OnRep_OnServer()
{
	if (Role == ROLE_Authority)
	{
		OnRep_ChangeTask();
	}
}

void ALDTasksManager::GarbageCollector()
{
	if (Role == ROLE_Authority)
	{
		for (auto& Task : m_Tasks.Tasks)
		{
			Task.MarkedActors.RemoveAll([&](const FLDTargetActor& TargetActor) {
				return TargetActor.MarkedActor == nullptr;
			});
		}

		if (IsUpdate || IsReserveCall)
		{
			// Just in case, start replication once after the last change
			IsReserveCall = IsUpdate;

			IsUpdate = false;
			MulticastUpdateTasks(m_Tasks);
			//UE_LOG(LogTemp, Warning, TEXT("ALDTasksManager::Update Tasks..."));
		}
	}
}

void ALDTasksManager::MulticastUpdateTasks_Implementation(FTasks Tasks)
{
	m_Tasks = Tasks;

	if (OnUpdateTask.IsBound())
		OnUpdateTask.Broadcast();
}
				