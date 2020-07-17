#pragma once

#include "Engine.h"
#include "LDTasksManager.generated.h"

UENUM(BlueprintType)
enum class EResult : uint8
{
	ExecTrue = 0,
	ExecFalse
};

UENUM(BlueprintType)
enum class ETaskState : uint8
{
	NonActive = 0,
	InProgress,
	Success,
	Failed,
};

USTRUCT(BlueprintType)
struct FLDTargetActor
{
	GENERATED_BODY()

	FLDTargetActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Target")
	int32			TeamId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Target")
	AActor*			MarkedActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Target")
	FName			SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Target")
	FName			TagName;
};

USTRUCT(BlueprintType)
struct FLDTask
{
	GENERATED_BODY()

	FLDTask();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	int32					Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	FText					Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	FText					Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	ETaskState				State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	bool					IsPrimary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	TArray<FLDTargetActor>	MarkedActors;
};

USTRUCT(BlueprintType)
struct FTasks
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LD Tasks")
	TArray<FLDTask>			Tasks;
};
/**
*
*/
UCLASS(Blueprintable, BlueprintType)
class LEVELLOGIC_API ALDTasksManager : public AActor
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChangeTask);

public:
	virtual void		BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	bool				CreateTask(
			int32 Id,
			FText Name = INVTEXT(""),
			FText Description = INVTEXT(""),
			ETaskState State = ETaskState::NonActive,
 			bool IsPrimary = true);

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	bool				RemoveTask(int32 Id);

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	void				SetState(int32 Id, ETaskState State);

	UFUNCTION(BlueprintCallable, Category = "LD Task", Meta = (ExpandEnumAsExecs = "Result"))
	void				CheckState(int32 Id, ETaskState State, bool NotEqual, EResult& Result);

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	bool				CompareState(int32 Id, ETaskState State, bool NotEqual);

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	void				UpdateTask(
			int32 Id,
			FText Name = INVTEXT(""),
			FText Description = INVTEXT(""),
			bool IsPrimary = true);

	UFUNCTION(BlueprintCallable, Category = "Task Manager")
	const TArray<FLDTask>&			GetTasks() const;

	UFUNCTION(BlueprintCallable, Category = "LD Task")
	const TArray<FLDTargetActor>&	GetMarkedActors(int32 Id) const;

	UFUNCTION(BlueprintCallable, Category = "LD MissionPoint")
	void				AddTaskMarkerToActor(int32 TaskId, int32 TeamId, AActor* Target, FName SocketName = FName(TEXT("")), FName TagName = FName(TEXT("")));

	UFUNCTION(BlueprintCallable, Category = "LD Mission Points")
	void				AddTaskMarkerToListActors(int32 TaskId, int32 TeamId, TArray<AActor*> Targets);

	UFUNCTION(BlueprintCallable, Category = "LD Mission Points")
	void				RemoveTaskMarkerFromActor(int32 TaskId, int32 TeamId, AActor* Target, FName SocketName = FName(TEXT("")), FName TagName = FName(TEXT("")));

	UFUNCTION(BlueprintCallable, Category = "LD Mission Points")
	void				RemoveAllTaskMarkersForActor(AActor* Target, int32 TeamId = 0);

	UFUNCTION(BlueprintCallable, Category = "LD Mission Points")
	void				RemoveAllTaskMarkers(int32 TaskId = -1);

	UFUNCTION()
	void				OnRep_ChangeTask();

	void				GarbageCollector();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LD Task")
	static ALDTasksManager*	GetTasksManager();

	void				OnRep_OnServer();

	UFUNCTION(NetMulticast, Reliable)
	void				MulticastUpdateTasks(FTasks Tasks);
	   
public:
	UPROPERTY(BlueprintAssignable)
	FOnChangeTask		OnUpdateTask;

	UPROPERTY(Transient, Replicated)
	int32				ShowTaskId;

	UPROPERTY()
	TArray<FLDTargetActor>	EmptyReturn;

protected:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ChangeTask)
	FTasks				m_Tasks;

	FTimerHandle		TimerHandle;

	bool				IsUpdate;

	bool				IsReserveCall;
};
