#pragma once

#include "ObjectiveMarkersBase.h"
#include "LDTasksManager.h"
#include "SlaveUIControllerBase.h"
#include "TasksViewer.generated.h"

class AFarHomeVRPlayerController;
class AFarHomeVRPlayerState;
class UFarHomeVRGameInstance;

USTRUCT(BlueprintType)
struct PROJECTX_API FTasksView
{
	GENERATED_USTRUCT_BODY()

	FTasksView();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	int32					Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	AObjectiveMarkersBase*	TaskMarker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	AActor*					TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	FName					SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	FName					TagName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	bool					IsLocal;
};

UCLASS(Blueprintable)
class PROJECTX_API ATasksViewer : public ASlaveUIControllerBase
{
	GENERATED_UCLASS_BODY()

public:
	//virtual void							BeginPlay() override;
	virtual void							Tick(float DeltaTime) override;

protected:
	FVector4								CalcCorrectedData(FVector Point);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "GetMarkerClass")
	void K2_GetMarkerClass(AActor* Actor, TSubclassOf<class AObjectiveMarkersBase>& MarkerClass);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "GetLobbyMarkerClass")
	void K2_GetLobbyMarkerClass(TSubclassOf<class AObjectiveMarkersBase>& MarkerClass);

	UFUNCTION(BlueprintCallable, Category = "TasksViewer")
	void									UpdateTasks();
	
	UFUNCTION()
	void									OnUpdateTasks();

	void									CalculateControllerPosition();

	void									UpdateObjectives(FVector m_ViewerLocation, FVector m_CameraLocation, float m_CurrentDistance);

	void									SetMarker(bool IsOutOfCircle, FVector m_TargetLocation, FVector m_CameraLocation, FVector4 DataVector = FVector4(0.f, 0.f, 0.f, 0.f));

	UFUNCTION(BlueprintCallable, Category = "TasksViewer")
	void									HideMarkers();

	UFUNCTION(BlueprintCallable, Category = "TasksViewer")
	void									ShowMarkers();

	UFUNCTION(BlueprintCallable, Category = "TasksViewer", DisplayName = "AddMarkerToActor")
	void									AddLocalMarkerToActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "TasksViewer", DisplayName = "RemoveMarkerToActor")
	void									RemoveLocalMarkerToActor(AActor* Actor);



public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float						DistanceToTasksViewer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float						ViewTasksRegionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float						EllipseRadiusRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float						MinDistanceScaleObjective;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSubclassOf<class AObjectiveMarkersBase>	DefaultMarkerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32						TeamId;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	TArray<FTasksView>				TasksView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	bool							IsEnableMarkers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TasksView")
	float							MaxTime;

	AObjectiveMarkersBase*			CurrentMarker;

	AActor*							CurrentTargetActor;

	bool							IsInitialize;

	float							DistanceToTarget;

	float							Scale;

	TArray<int32>					RemoveIndex;

	float							CurrentRadius;

	UPROPERTY()
	APlayerController*				PlayerController;

	UPROPERTY()
	AFarHomeVRPlayerState*			PlayerState;

	UPROPERTY()
	UFarHomeVRGameInstance*			GameInstance;

	float							DistanceToPlane;

	UPROPERTY()
	UDataTable*						ObjectiveClasses;
};