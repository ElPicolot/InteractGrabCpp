// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractGrabCppCharacter.h"
#include "InteractGrabCppProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "DrawDebugHelpers.h"
#include "ProfilingDebugging/StallDetector.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AInteractGrabCppCharacter

AInteractGrabCppCharacter::AInteractGrabCppCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	HoldingComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HoldingComponent"));
	//HoldingComponent->RelativeLocation.X=50.0f;
	HoldingComponent->SetRelativeLocation(FVector(50.0f,0.0f,0.0f));
	//HoldingComponent->SetRelativeLocation.X(50.0f);
	
	HoldingComponent->SetupAttachment(RootComponent);
	//Pas vr du coup chiant

	CurrentItem = NULL;
	bCanMove = true;
	bInspecting = false;
}

void AInteractGrabCppCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	UE_LOG(LogTemp,Warning,TEXT("Marche  !"));


	pitchMax=GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax;
	pitchMin=GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin;
	
}

void AInteractGrabCppCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Start = FirstPersonCameraComponent->GetComponentLocation();
	ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	End = ((ForwardVector* 200.0f)+ Start);

	//DrawDebugLine(GetWorld(),Start,End,FColor::Green,false,ECC_Visibility);
	DrawDebugLine(GetWorld(),Start,End,FColor::Green,false,1,0,1);

	if(!bHoldingItem)
	{
		if(GetWorld()->LineTraceSingleByChannel(Hit,Start, End ,ECC_Visibility,DefaultComponentQueryParams,DefaultResponseParams))
		{
			if(Hit.GetActor()->GetClass()->IsChildOf(APickup::StaticClass()))
			{
				CurrentItem = Cast<APickup>(Hit.GetActor());
				
			}
		}
		else
		{
			CurrentItem =NULL;
		}
	}
	

	if(bInspecting)
	{
		if(bHoldingItem)
		{
			FirstPersonCameraComponent->SetFieldOfView(FMath::Lerp(FirstPersonCameraComponent->FieldOfView,90.0f, 0.1f));
			HoldingComponent->SetRelativeLocation(FVector(0.0f,50.0f,50.0f));
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax=179.900002f;
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin=-179.900002f;
			CurrentItem->RotateActor();
		}
		else
		{
				FirstPersonCameraComponent->SetFieldOfView(FMath::Lerp(FirstPersonCameraComponent->FieldOfView,45.0f, 0.1f));
		}
	}
	else
	{
		FirstPersonCameraComponent->SetFieldOfView(FMath::Lerp(FirstPersonCameraComponent->FieldOfView,90.0f, 0.1f));
		
		if(bHoldingItem)
		{
			HoldingComponent->SetRelativeLocation(FVector(50.0f,0.0f,0.0f));
			
		}
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AInteractGrabCppCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AInteractGrabCppCharacter::Move);

		// Action à voir
		
		EnhancedInputComponent->BindAction(ActionAction, ETriggerEvent::Triggered, this, &AInteractGrabCppCharacter::OnAction);
		
		EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Triggered, this, &AInteractGrabCppCharacter::OnInspect);
		EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Triggered, this, &AInteractGrabCppCharacter::OnInspectReleased);
		

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AInteractGrabCppCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AInteractGrabCppCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
		//add b CanMove si jamais pdt qu on inspect on ne doit pas bouger
	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AInteractGrabCppCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AInteractGrabCppCharacter::OnAction()
{
	if(CurrentItem && !bInspecting)
	{
		ToggleItemPickup();
	}
}

void AInteractGrabCppCharacter::OnInspect()
{
	if(bHoldingItem)
	{
		LastRotation = GetControlRotation();
		ToggleMovement();
	}
	else
	{
		bInspecting = true;
	}
}

void AInteractGrabCppCharacter::OnInspectReleased()
{
	if(bInspecting && bHoldingItem)
	{
		GetController()->SetControlRotation(LastRotation);
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax = pitchMax;
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin = pitchMin;
		ToggleMovement();
	}
	else
	{
		bInspecting = false;
	}
}

void AInteractGrabCppCharacter::ToggleMovement()
{
	bCanMove = !bCanMove;
	bInspecting = !bInspecting;
	FirstPersonCameraComponent->bUsePawnControlRotation = !FirstPersonCameraComponent->bUsePawnControlRotation;
	bUseControllerRotationYaw = !bUseControllerRotationYaw;
}

void AInteractGrabCppCharacter::ToggleItemPickup()
{
	if(CurrentItem)
	{
		bHoldingItem = !bHoldingItem;
		CurrentItem->Pickup();

		if(!bHoldingItem)
		{
			CurrentItem = NULL;
		}
	}
}

