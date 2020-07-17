#include "FarHomeVR.h"
#include "FarHomeVRPlayerState.h"
#include "ShooterCharacterBase.h"
#include "TasksViewer.h"

FTasksView::FTasksView()
: Id(0)
, TaskMarker(nullptr)
, TargetActor(nullptr)
, IsLocal(false)
{}

ATasksViewer::ATasksViewer(const FObjectInitializer& P) : Super(P)
, DistanceToTasksViewer(100.f)
, ViewTasksRegionRadius(60.f)
, EllipseRadiusRatio(1.f)
, MinDistanceScaleObjective(50.f)
, IsEnableMarkers(true)
, MaxTime(60.f)
, IsInitialize(false)
{
}

void ATasksViewer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsEnableMarkers)
	{
		if (IsInitialize)
		{
			if (TasksView.Num() <= 0)
				return;

			if (MaxTime >= 0.f)
			{
				int32 TempTeamID;
				PlayerState->GetTeamID(TempTeamID);
				MaxTime = MaxTime - DeltaTime;
				if (TempTeamID != TeamId)
				{
					UE_LOG(LogTemp, Warning, TEXT("TasksViewer. Changed TeamID. Now TeamID = %i, Time = %f"), TeamId, MaxTime);
					TeamId = TempTeamID;
					OnUpdateTasks();
				}
				else
					CalculateControllerPosition();
			}
			else
			{
				CalculateControllerPosition();
			}
		}
		else
		{
			auto TaskManager = ALDTasksManager::GetTasksManager();
			if (TaskManager == nullptr)
				return;

			PlayerController = UUtil::GetFirstLocalPlayerController();
			if (PlayerController == nullptr)
				return;

			PlayerState = Cast<AFarHomeVRPlayerState>(PlayerController->PlayerState);
			if (PlayerState == nullptr)
				return;

			PlayerState->GetTeamID(TeamId);
			TaskManager->OnUpdateTask.AddDynamic(this, &ATasksViewer::OnUpdateTasks);
			OnUpdateTasks();
			
			UE_LOG(LogTemp, Warning, TEXT("TasksViewer. Set TeamID. TeamID = %i"), TeamId);

			IsInitialize = true;
		}
	}
	else
	{
		for (auto& TaskView : TasksView)
		{
			FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
			TaskView.TaskMarker->DetachFromActor(Rules);
			TaskView.TaskMarker->Destroy();
		}
		TasksView.Empty();
	}
}

FVector4 ATasksViewer::CalcCorrectedData(FVector Point)
{
	float Angle;
	if (FMath::IsNearlyZero(Point.Y))
		Angle = FMath::DegreesToRadians(90.f);
	else
		Angle = FMath::Atan(FMath::Abs(Point.Z / Point.Y));

	float AngleInDegree = FMath::RadiansToDegrees(Angle);
	float Sin = FMath::Sin(Angle);
	float Cos = FMath::Cos(Angle);

	float Coeff = FMath::Square(CurrentRadius) * EllipseRadiusRatio / 
				FMath::Sqrt(FMath::Square(CurrentRadius) * Sin * Sin + FMath::Square(CurrentRadius * EllipseRadiusRatio) * Cos * Cos);
		
	float DeltaX = Coeff * Cos;
	float DeltaY = Coeff * Sin;

	if (DistanceToPlane > 0.f)
	{
		if (Point.Y <= 0.f)
		{
			DeltaX = 0.f - ViewTasksRegionRadius;
			DeltaY = 0.f;
			AngleInDegree = -90.f;
		}
		else
		{
			DeltaX = ViewTasksRegionRadius;
			DeltaY = 0.f;
			AngleInDegree = 90.f;
		}
	}
	else
	{
		if (Point.Y <= 0.f && Point.Z <= 0.f)
		{
			DeltaX = -DeltaX;
			DeltaY = -DeltaY;
			AngleInDegree = -90.f - AngleInDegree;
		}
		if (Point.Y >= 0.f && Point.Z <= 0.f)
		{
			DeltaY = -DeltaY;
			AngleInDegree = 90.f + AngleInDegree;
		}
		if (Point.Y >= 0.f && Point.Z >= 0.f)
		{
			AngleInDegree = 90.f - AngleInDegree;
		}
		if (Point.Y <= 0.f && Point.Z >= 0.f)
		{
			DeltaX = -DeltaX;
			AngleInDegree = AngleInDegree - 90.f;
		}
	}

	return FVector4(DeltaX, DeltaY, 0.f, AngleInDegree);
}

void ATasksViewer::OnUpdateTasks()
{
	auto TasksManager = ALDTasksManager::GetTasksManager();
	if (TasksManager == nullptr)
		return;

	auto Tasks = TasksManager->GetTasks();
	int32 TeamID;
	PlayerState->GetTeamID(TeamID);

	// Remove deleted objectives
	for (auto& Task : Tasks)
	{
		if (TasksView.Num() > 0)
		{
			for (int32 i = TasksView.Num() - 1; i >= 0; --i)
			{
				if (!TasksView[i].IsLocal)
				{
					bool IsDelete = true;
					for (auto& MarkedActor : Task.MarkedActors)
					{
						if (TasksView[i].TargetActor == MarkedActor.MarkedActor
							&& TasksView[i].SocketName == MarkedActor.SocketName
							&& TasksView[i].TagName == MarkedActor.TagName
							&& Task.State == ETaskState::InProgress
							&& TasksView[i].Id == Task.Id
							&& TeamID == MarkedActor.TeamId)
						{
							IsDelete = false;
						}
					}
					if (IsDelete)
					{
						if (TasksView[i].TaskMarker != nullptr)
						{
							FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
							TasksView[i].TaskMarker->DetachFromActor(Rules);
							TasksView[i].TaskMarker->K2_OnHideAll();
							TasksView[i].TaskMarker->Destroy();
						}
						TasksView.RemoveAt(i);
					}
				}
			}
		}
	}

	// Added new objectives
	for (auto& Task : Tasks)
	{
		if (Task.State == ETaskState::InProgress)
		{
			for (auto& MarkedActor : Task.MarkedActors)
			{
				bool IsTaskExist = false;
				for (auto& TaskView : TasksView)
				{
					if (TaskView.TargetActor == MarkedActor.MarkedActor
						&& TaskView.SocketName == MarkedActor.SocketName
						&& TaskView.TagName == MarkedActor.TagName
						&& TaskView.Id == Task.Id
						&& (TeamID == MarkedActor.TeamId) // || MarkedActor.TeamId == -1)
						&& TaskView.TargetActor != nullptr)
					{
						IsTaskExist = true;
					}
				}

				if (!IsTaskExist)
				{
					if (DefaultMarkerClass != nullptr)
					{
						TSubclassOf<class AObjectiveMarkersBase> MarkerClass;
						K2_GetMarkerClass(MarkedActor.MarkedActor, MarkerClass);

						auto Marker = GetWorld()->SpawnActor<AObjectiveMarkersBase>(MarkerClass);
						FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
						Marker->AttachToActor(this, Rules);

						if (IsEnableMarkers)
						{
							Marker->K2_OnHideAll();
							FTasksView NewMarker;
							NewMarker.Id = Task.Id;
							NewMarker.TaskMarker = Marker;
							NewMarker.SocketName = MarkedActor.SocketName;
							NewMarker.TargetActor = MarkedActor.MarkedActor;
							NewMarker.TagName = MarkedActor.TagName;
							NewMarker.IsLocal = false;

							TasksView.Add(NewMarker);
						}
					}
				}
			}
		}
	}
}

void ATasksViewer::CalculateControllerPosition()
{
	if (TasksView.Num() > 0)
	{
		for (int32 i = TasksView.Num() - 1; i >= 0; --i)
		{
			bool IsDelete = false;
			if (TasksView[i].TargetActor != nullptr)
			{
				auto Character = Cast<AShooterCharacterBase>(TasksView[i].TargetActor);
				if (Character != nullptr && Character->IsKilled())
				{
					IsDelete = true;
				}
			}
			else
				IsDelete = true;

			if (TasksView[i].TaskMarker != nullptr && IsDelete)
			{
				FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
				TasksView[i].TaskMarker->DetachFromActor(Rules);
				TasksView[i].TaskMarker->Destroy();
				TasksView.RemoveAt(i);
			}
		}
	}
	
	auto CameraManager = PlayerController->PlayerCameraManager;
	if (CameraManager != nullptr)
	{
		float CurrentDistance = DistanceToTasksViewer;
		CurrentRadius = ViewTasksRegionRadius;

		auto CameraLocation = CameraManager->GetCameraLocation();
		auto ForwardVector = CameraManager->GetActorForwardVector();// CameraManager->GetCameraRotation().Vector();

		auto ViewerLocation = ForwardVector * DistanceToTasksViewer + CameraLocation;

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.bTraceComplex = false;
		
		bool IsHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, ViewerLocation, ECollisionChannel::ECC_WorldStatic, QueryParams);
		if (IsHit && (CameraLocation - HitResult.ImpactPoint).Size() < 0)
		{
			auto Distance = (CameraLocation - HitResult.ImpactPoint).Size();
			CurrentDistance = Distance < MinDistanceScaleObjective ? MinDistanceScaleObjective : Distance;

			CurrentRadius = ViewTasksRegionRadius / DistanceToTasksViewer * CurrentDistance;
			ViewerLocation = ForwardVector * CurrentDistance + CameraLocation;
		}

		SetActorLocation(ViewerLocation);
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(ViewerLocation, CameraLocation));

		UpdateObjectives(ViewerLocation, CameraLocation, CurrentDistance);
	}
}

void ATasksViewer::UpdateObjectives(FVector m_ViewerLocation, FVector m_CameraLocation, float m_CurrentDistance)
{
	bool IsOutOfCircle = false;

	auto BasePoint = m_ViewerLocation;
	auto Normal = (m_CameraLocation - m_ViewerLocation).GetSafeNormal();
	auto Plane = FPlane(m_ViewerLocation, Normal);
	FVector TargetLocation = FVector(0.f, 0.f, 0.f);

	for (auto& TaskView : TasksView)
	{
		if (TaskView.TaskMarker != nullptr && TaskView.TargetActor != nullptr)
		{
			CurrentTargetActor = TaskView.TargetActor;
			CurrentMarker = TaskView.TaskMarker;

			// Find Target Location
			bool ExistSocketName = false;
			if (TaskView.SocketName != TEXT(""))
			{
				auto Character = Cast<ACharacter>(CurrentTargetActor);
				if (Character != nullptr)
				{
					auto Mesh = Character->GetMesh();
					if (Character != nullptr)
					{
						TargetLocation = Mesh->GetSocketLocation(TaskView.SocketName);
						ExistSocketName = true;
					}
				}
			}
			if (!ExistSocketName && TaskView.TagName != TEXT(""))
			{
				auto Components = CurrentTargetActor->GetComponentsByTag(USceneComponent::StaticClass(), TaskView.TagName);
				if (Components.Num() > 0)
				{
					auto SceneComponent = Cast<USceneComponent>(Components[0]);
					TargetLocation = SceneComponent->GetComponentLocation();
				}
			}
			else if (!ExistSocketName)
			{
				TargetLocation = CurrentTargetActor->GetActorLocation();
			}

			DistanceToTarget = (TargetLocation - m_CameraLocation).Size();
			float Angle = 0.f;
			DistanceToPlane = (TargetLocation.X * Plane.X + TargetLocation.Y * Plane.Y + TargetLocation.Z * Plane.Z) - Plane.W;

			FVector Intersect = FMath::LinePlaneIntersection(m_CameraLocation, TargetLocation, Plane);
			float Dist = (Intersect - BasePoint).Size();

			if (DistanceToPlane <= m_CurrentDistance && Dist < CurrentRadius)
			{
				IsOutOfCircle = false;
			}
			else
			{
				IsOutOfCircle = true;
				Intersect = (FVector::PointPlaneProject(TargetLocation, BasePoint, Normal) - BasePoint).GetSafeNormal() * CurrentRadius + BasePoint;
			}

			auto Delta = GetActorTransform().InverseTransformPosition(Intersect);

			if (FMath::Square(Delta.Y) / FMath::Square(CurrentRadius) + FMath::Square(Delta.Z) / FMath::Square(CurrentRadius * EllipseRadiusRatio) > 1.f)
				IsOutOfCircle = true;

			FVector4 DataVector;
			if (IsOutOfCircle)
				DataVector = CalcCorrectedData(Delta);
			else
				DataVector = FVector4(Delta.Y, Delta.Z, 0.f, 0.f);

			SetMarker(IsOutOfCircle, TargetLocation, m_CameraLocation, DataVector);
		}
	}
}

void ATasksViewer::SetMarker(bool IsOutOfCircle, FVector m_TargetLocation, FVector m_CameraLocation, FVector4 DataVector /*= FVector4(0.f, 0.f, 0.f, 0.f)*/)
{
	if (CurrentMarker->IsShow)
	{
		
		if (IsOutOfCircle)
		{
			CurrentMarker->K2_OnHideInVisible();
			CurrentMarker->SetActorRelativeLocation(FVector(0.f, DataVector.X, DataVector.Y));
		}
		else
		{
			CurrentMarker->K2_OnHideInUnvisible();
			CurrentMarker->SetActorLocation(m_CameraLocation + (m_TargetLocation - m_CameraLocation).GetSafeNormal() * DistanceToTarget);
		}
		CurrentMarker->SetActorRelativeRotation(UKismetMathLibrary::ComposeRotators(FRotator(90.f, 180.f, 0.f), FRotator(0.f, 0.f, DataVector.W)));
		CurrentMarker->K2_OnDrawMarker(IsOutOfCircle, DistanceToTarget, CurrentTargetActor);
	}
}

void ATasksViewer::HideMarkers()
{
	for (auto& TaskView : TasksView)
	{
		TaskView.TaskMarker->IsShow = false;
		TaskView.TaskMarker->K2_OnHideAll();
		if (TaskView.TargetActor != nullptr)
		{
			auto Components = TaskView.TargetActor->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("Marker"));
			if (Components.Num() > 0)
			{
				auto Component = Cast<USceneComponent>(Components[0]);
				if (Component != nullptr)
					Component->SetVisibility(false);
			}
		}
	}
}

void ATasksViewer::ShowMarkers()
{
	for (auto& TaskView : TasksView)
	{
		TaskView.TaskMarker->IsShow = false;
		TaskView.TaskMarker->K2_OnShowAll();
		if (TaskView.TargetActor != nullptr)
		{
			auto Components = TaskView.TargetActor->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("Marker"));
			if (Components.Num() > 0)
			{
				auto Component = Cast<USceneComponent>(Components[0]);
				if (Component != nullptr)
					Component->SetVisibility(true);
			}
		}
	}
}

void ATasksViewer::AddLocalMarkerToActor(AActor* Actor)
{
	TSubclassOf<class AObjectiveMarkersBase> MarkerClass;
	K2_GetLobbyMarkerClass(MarkerClass);

	if (IsEnableMarkers && MarkerClass != nullptr && Actor != nullptr)
	{
		auto Marker = GetWorld()->SpawnActor<AObjectiveMarkersBase>(MarkerClass);
		FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
		Marker->AttachToActor(this, Rules);

		Marker->K2_OnHideAll();
		FTasksView NewMarker;
		NewMarker.Id = 0;
		NewMarker.TaskMarker = Marker;
		NewMarker.SocketName = TEXT("");
		NewMarker.TargetActor = Actor;
		NewMarker.TagName = TEXT("");
		NewMarker.IsLocal = true;

		TasksView.Add(NewMarker);

		auto Components = Actor->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("Marker"));
		if (Components.Num() > 0)
		{
			auto Component = Cast<USceneComponent>(Components[0]);
			if (Component != nullptr)
				Component->SetVisibility(true);
		}
	}
}

void ATasksViewer::RemoveLocalMarkerToActor(AActor* Actor)
{
	int32 DeleteIndex = -1;
	bool IsDelete = false;

	for (int32 i = 0; i < TasksView.Num(); ++i)
	{
		if (TasksView[i].TargetActor == Actor && TasksView[i].IsLocal)
		{
			FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
			TasksView[i].TaskMarker->DetachFromActor(Rules);
			TasksView[i].TaskMarker->K2_OnHideAll();
			TasksView[i].TaskMarker->Destroy();

			DeleteIndex = i;
			IsDelete = true;

			auto Components = Actor->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("Marker"));
			if (Components.Num() > 0)
			{
				auto Component = Cast<USceneComponent>(Components[0]);
				if (Component != nullptr)
					Component->SetVisibility(false);
			}

			break;
		}
	}
	
	if (IsDelete)
		TasksView.RemoveAt(DeleteIndex);
}

void ATasksViewer::UpdateTasks()
{
	OnUpdateTasks();
}
