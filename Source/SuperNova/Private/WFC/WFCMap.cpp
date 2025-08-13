
#include "WFC/WFCMap.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Array.h"
#include "Containers/Queue.h"
#include "GameFramework/GameModeBase.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Components/BrushComponent.h"
#include "Enemy/Enemy.h"
#include "Engine/BrushBuilder.h"
#include "Containers/Set.h"
#include "Engine/TargetPoint.h"
#include "Breakable/BreakableActor.h"
#include "GameInstance/SuperNovaGameInstance.h"
#include "Items/Weapons/Weapon.h"
#include "Items/EvacuationPoint.h"
#include "Character/SuperNovaCharacter.h"
#include "Items/Grass.h"
#include "Items/Portal.h"

AWFCMap::AWFCMap()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AWFCMap::BeginPlay()
{
	Super::BeginPlay();

	// ��ȡGameInstance
	USuperNovaGameInstance* GameInstance = Cast<USuperNovaGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		//UE_LOG(LogTemp, Warning, TEXT("%d:%d"), GameInstance->MapSize, GameInstance->Mode);
		if (GameInstance->MapSize == 0)//С 40 * 40
		{
			MapSize = FVector2D(40, 40);
		}
		else if (GameInstance->MapSize == 1)
		{
			MapSize = FVector2D(50, 50);
		}
		else
		{
			MapSize = FVector2D(60, 60);
		}
		if (GameInstance->Mode == 1)
		{
			bHardMode = true;
		}
	}
	/** ���ɵ�ͼ **/
	InitializeMap();
	//����ж����ͨ���򲻷���������������
	int32 step = 0;
	while (!CheckConnectivity())
	{
		UE_LOG(LogTemp, Warning, TEXT("step:%d"), step);
		step += 1;
		for (AWFCTile* tile : AllTiles)
		{
			tile->bIsCollapsed = false;
			InitializeTileStates(tile);
		}
		BeginWFC();
	}
	/** ���ɵ�ͼ **/

	//���ɵ�������
	AddNavigationVolume();

	//ϸ���Զ����㷨
	BeginCellularAutomata();

	//��������
	AddGrass();

	//���λ���������
	SpawnPlayer();

	//���ɿ��Դ��Ƶ���������
	SpawnBreakableActor();

	//����AI
	SpawnEnemy();

	//���ɳ����
	SpawnEvacuationPoint();

	//���ɴ�����
	SpawnPortal();
}

void AWFCMap::InitializeMap()
{
	// �������е� ��Ƭ

	// ������ֵ 

	// �ж��Ƿ�ȫ������
		//��ȡ��ֵ��С����Ƭ
			//�������������޸Ķ���
			//�Ӷ�����ȡ����Ƭֱ������Ϊ��
			// ��ȡ����Χ����Ƭ
				//�����ܺ���Ƭ��ӵ�״̬ɾ�������¼�����ֵ���������޸Ķ���
			//�����Ƿ����޽�
				//����״̬
				//����

	//�ж���ͨ��
	//���ɳɹ�
	//����ʧ����������

	//����
	ReadTable();

	//���ص�ǰ Actor ������ռ䣨ȫ������ϵ���е�λ��
	ActorLocation = GetActorLocation();
	//���ص�ǰ Actor ���������ű���
	ActorScale = GetActorScale3D();

	ScaleX = TileSize * ActorScale.X;
	ScaleY = TileSize * ActorScale.Y;


	// ��ʼ�����е���Ƭ
	for (int32 X = 0; X <= MapSize.X + 1; X++)
	{
		for (int32 Y = 0; Y <= MapSize.Y + 1; Y++)
		{

			//������Ƭ����λ��
			//FVector SpawnLocation = FVector(ActorLocation.X + X * TileSize , ActorLocation.Y + Y * TileSize , ActorLocation.Z );

			FVector SpawnLocation = FVector(ActorLocation.X + ScaleX * X, ActorLocation.Y + ScaleY * Y, ActorLocation.Z);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AWFCTile* NewTile = GetWorld()->SpawnActor<AWFCTile>(AWFCTile::StaticClass(), SpawnLocation, GetActorRotation(), SpawnParams);
			if (NewTile)
			{
				//��������
				NewTile->MapCoordinate = FVector2D(X, Y);

				//��ʼ����Ƭ�Ŀ���״̬
				InitializeTileStates(NewTile);

				//��ӵ�����
				AllTiles.Add(NewTile);

				//UE_LOG(LogTemp, Log, TEXT("������Ƭ��.. (%d, %d)"), X, Y);

			}
		}
	}
	//UE_LOG(LogTemp, Log, TEXT("�������"));

	BeginWFC();
}

void AWFCMap::ReadTable()
{
	// �����������ַ��������ڵ�����Ϣ
	static const FString ContextString(TEXT("Fetching All Rows in Tile Stats Table"));



	if (ConnectStatsTable && TileStatsTable)
	{
		// ��ȡ���е����ӱ�����
		TArray<FConnectType*> AllConnectStatsPtr;
		ConnectStatsTable->GetAllRows(ContextString, AllConnectStatsPtr);
		for (FConnectType* Ptr : AllConnectStatsPtr)
		{
			if (Ptr)
			{
				AllConnectStats.Add(*Ptr);
			}
		}


		// ��ȡ���е���Ƭ����
		TArray<FTileType*> AllTileStatsPtr;
		TileStatsTable->GetAllRows(ContextString, AllTileStatsPtr);
		for (FTileType* Ptr : AllTileStatsPtr)
		{
			if (Ptr)
			{
				AllTileStats.Add(*Ptr);
			}
		}
	}
}

//��ʼ����Ƭ�Ŀ���״̬
void AWFCMap::InitializeTileStates(AWFCTile* TargetTile)
{
	TargetTile->MaybeStates.Empty();
	//�������Ƭֻ�п�״̬,��֤���ɵĵ�ͼ�Ƿ��״̬
	if (TargetTile->MapCoordinate.X == 0 || 
		TargetTile->MapCoordinate.X == MapSize.X + 1 ||
		TargetTile->MapCoordinate.Y == 0 ||
		TargetTile->MapCoordinate.Y == MapSize.Y + 1)
	{
		//����״̬
		TargetTile->MaybeStates.Add(AllTileStats[0]);
	}
	else
	{
		//���ÿ��ܵ�״̬
		//NewTile->MaybeStates = AllTileStats;
		//ÿ�����͵���Ƭ��ͬ�������ת �ǲ�ͬ��״̬
		for (FTileType type : AllTileStats)
		{

			FGameplayTag Old_L_ID = type.Connect_L_ID;
			FGameplayTag Old_R_ID = type.Connect_R_ID;
			FGameplayTag Old_F_ID = type.Connect_F_ID;
			FGameplayTag Old_B_ID = type.Connect_B_ID;

			//��ͬ�����״̬
			TArray<FTileType> DirectionStates;
			for (int32 i : {0, 1, 2, 3})
			{
				//�����ж�ѭ��
				int32 break_i = -1;
				//�ж��Ƿ�Ϊ��Ա� ��ͬ������,�ж��Ƿ�Ϊ4����ȫ��ͬ
				if ((Old_L_ID == Old_R_ID && Old_F_ID == Old_B_ID) && Old_F_ID == Old_L_ID)
				{
					//4����ȫ��ͬ ֻ��һ��״̬ ����ת
					break_i = 1;
				}
				if ((Old_L_ID == Old_R_ID && Old_F_ID == Old_B_ID) && Old_F_ID != Old_L_ID)
				{
					//�ԳƱ���ͬ ֻ������״̬ ֻ��תһ��	
					break_i = 2;
				}
				if (i == break_i)
				{
					break;
				}


				if (i == 0)
				{
					//����ת
					type.DirectionOfRotation = 0;
				}
				else if (i == 1)
				{
					//˳ʱ����ת 90
					type.DirectionOfRotation = 1;
					type.Connect_F_ID = Old_L_ID;
					type.Connect_R_ID = Old_F_ID;
					type.Connect_B_ID = Old_R_ID;
					type.Connect_L_ID = Old_B_ID;
				}
				else if (i == 2)
				{
					//��ת180
					type.DirectionOfRotation = 2;
					type.Connect_F_ID = Old_B_ID;
					type.Connect_R_ID = Old_L_ID;
					type.Connect_B_ID = Old_F_ID;
					type.Connect_L_ID = Old_R_ID;
				}
				else if (i == 3)
				{
					//��ת270
					type.DirectionOfRotation = 3;
					type.Connect_F_ID = Old_R_ID;
					type.Connect_R_ID = Old_B_ID;
					type.Connect_B_ID = Old_L_ID;
					type.Connect_L_ID = Old_F_ID;
				}
				DirectionStates.Add(type);
			}
			//����״̬
			TargetTile->MaybeStates.Append(DirectionStates);
		}
	}

	//������ֵ
	TargetTile->Entropy = CalculateEntropy(TargetTile->MaybeStates);
}

void AWFCMap::BeginWFC()
{
	//��ȡ����û���ݵ���Ƭ 
	TArray<AWFCTile*> NotCollapseTiles;
	for (AWFCTile* tile : AllTiles)
	{
		if (!tile->bIsCollapsed)
		{
			NotCollapseTiles.Add(tile);
		}
	}

	int32 NotCollapseTilesCount = NotCollapseTiles.Num();
	UE_LOG(LogTemp, Log, TEXT("û���ݵ���Ƭ����: %d"), NotCollapseTilesCount);

	while (NotCollapseTilesCount != 0)
	{
		UE_LOG(LogTemp, Log, TEXT("��ʼ��������: %d "), NotCollapseTilesCount);
		//�Ƿ��˻�
		bool IsRestoreTileStates = false;
		//��ȡ��С��ֵ��Ƭ
		AWFCTile* MinEntropyTile = GetTileWithMinimumEntropy(NotCollapseTiles);
		if (MinEntropyTile == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Error: No tile found with minimum entropy."));
			break;
		}

		//����״̬, ÿ������֮ǰ���浱ǰ״̬
		CacheTileStates(AllTiles);

		//̮��
		MinEntropyTile->Collapse();

		TQueue<AWFCTile*> ChangingTiles;
		ChangingTiles.Enqueue(MinEntropyTile);
		AWFCTile* TileNeedToChange = nullptr;
		while (ChangingTiles.Dequeue(TileNeedToChange))
		{
			//��ȡ�ڱ���Ƭ
			TArray<AWFCTile*> NeighboringTiles = GetNeighboringTiles(TileNeedToChange);

			for (AWFCTile* NeighTile : NeighboringTiles)
			{
				TArray<FTileType> MaybeStates = NeighTile->MaybeStates;
				int32 ConnectCount = MaybeStates.Num();
				//TileNeedToChange�Ѿ�����
				if (TileNeedToChange->bIsCollapsed)
				{
					//����������Ƭ������ڱ���Ƭ �ߵ�����ID
					FGameplayTag ConnectID = TileNeedToChange->GetConnectTagToTile(NeighTile);

					//ɾ��������ӵ�״̬
					for (int32 i = MaybeStates.Num() - 1; i >= 0; i--)
					{
						//�����ڱ���Ƭ�����������Ƭ �ߵ�����ID
						FGameplayTag IsCanConnectID = NeighTile->GetConnectTagToState(MaybeStates[i], TileNeedToChange);
						//�ж��Ƿ�������
						if (!CanConnect(ConnectID, IsCanConnectID))
						{
							//������ӵ��Ƴ���
							MaybeStates.RemoveAt(i);
						}
					}
				}
				//TileNeedToChange��δ����
				else
				{
					TArray<int32> StatesIndex; //���ڼ�¼��Щ״̬�ǿ��е�
					StatesIndex.Init(0, MaybeStates.Num());
					for (int32 i = TileNeedToChange->MaybeStates.Num() - 1; i >= 0; i--)
					{
						FTileType NeedToChangestate = TileNeedToChange->MaybeStates[i];
						//������Ƭ��ǰ״̬������ڱ���Ƭ�ߵ�����ID
						FGameplayTag ConnectID = TileNeedToChange->GetConnectTagToState(NeedToChangestate, NeighTile);

						//��¼�������ӵ�״̬
						for (int32 j = MaybeStates.Num() - 1; j >= 0; j--)
						{
							//�����ڱ���Ƭ�����������Ƭ�ߵ�����ID
							FGameplayTag IsCanConnectID = NeighTile->GetConnectTagToState(MaybeStates[j], TileNeedToChange);
							//�ж��Ƿ�������
							if (CanConnect(ConnectID, IsCanConnectID))
							{
								StatesIndex[j] = 1;
							}
						}
					}
					//ɾ�������е�״̬
					for (int32 i = MaybeStates.Num() - 1; i >= 0; i--)
					{
						if (StatesIndex[i] == 0)
						{
							MaybeStates.RemoveAt(i);
						}
					}
				}
				//����AllTiles�е���Ƭ��Ϣ
				NeighTile->MaybeStates = MaybeStates;

				if (MaybeStates.Num() == 0)
				{
					//�޽� ����
					//FMapRow Cache = HistoryMap[HistoryMap.Num() - 1];
					//����û���ݵ�����
					RestoreTileStates(AllTiles);
					//NotCollapseTilesCount++;
					//Ѱ�ұ������Ƭ���˻�����
					//AllTiles = Cache.Tiles;
					IsRestoreTileStates = true;
					break;
				}

				//���¼�����ֵ
				if (ConnectCount != MaybeStates.Num())
				{
					NeighTile->Entropy = CalculateEntropy(MaybeStates);
					ChangingTiles.Enqueue(NeighTile);
				}
			}
			if (IsRestoreTileStates) break;
		}
		if (!IsRestoreTileStates)
		{
			//����û���ݵ�����
			NotCollapseTilesCount--;

		}
		//����û���ݵ���Ƭ���� 
		NotCollapseTiles.Empty();
		for (AWFCTile* tile : AllTiles)
		{
			if (!tile->bIsCollapsed)
			{
				NotCollapseTiles.Add(tile);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("��ɴ���"));
}

//����Ȩ��
float AWFCMap::CalculateEntropy(TArray<FTileType> TileTypes)
{
	float Entropy = 0.0f;
	float TotalWeight = 0.0f;
	for (const FTileType& State : TileTypes)
	{
		TotalWeight += State.Weight;
	}

	for (const FTileType& State : TileTypes)
	{
		float Probability = State.Weight / TotalWeight;
		Entropy -= Probability * FMath::Log2(Probability);
	}
	return Entropy;
}

//��ȡ��С����ֵ
AWFCTile* AWFCMap::GetTileWithMinimumEntropy(TArray<AWFCTile*> Tiles)
{
	// ��ӷ�����
	if (Tiles.Num() == 0)
	{
		return nullptr;
	}

	AWFCTile* MinEntropyTile = Tiles[0];

	for (AWFCTile* tile : Tiles)
	{
		if (tile->Entropy < MinEntropyTile->Entropy)
		{
			MinEntropyTile = tile;
		}
	}
	return MinEntropyTile;
}

TArray<AWFCTile*> AWFCMap::GetNeighboringTiles(AWFCTile* TargetTile)
{
	TArray<AWFCTile*> Neighs;

	FVector2D TargetCoordinates = TargetTile->MapCoordinate;

	//��
	int32 Index;
	if (TargetCoordinates.Y > 0)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		//�����Ѿ����ݵ���Ƭ
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	// ��
	if (TargetCoordinates.Y < MapSize.Y + 1)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		//�����Ѿ����ݵ���Ƭ
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	//ǰ
	if (TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		//�����Ѿ����ݵ���Ƭ
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	//��
	if (TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		//�����Ѿ����ݵ���Ƭ
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	return Neighs;
}

int32 AWFCMap::GetNeighboringGrassTilesNumber(AWFCTile* TargetTile)
{
	int32 GrassNumber = 0;
	FVector2D TargetCoordinates = TargetTile->MapCoordinate;

	//��
	int32 Index;
	if (TargetCoordinates.Y > 0)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//��
	if (TargetCoordinates.Y < MapSize.Y + 1)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//ǰ
	if (TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//��
	if (TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//��ǰ
	if (TargetCoordinates.Y > 0 && TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//��ǰ
	if (TargetCoordinates.Y < MapSize.Y + 1 && TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//���
	if (TargetCoordinates.Y > 0 && TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//�Һ�
	if (TargetCoordinates.Y < MapSize.Y + 1 && TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	return GrassNumber;
}

bool AWFCMap::CanConnect(FGameplayTag ID, FGameplayTag ConnectID)
{
	bool bIsCanConnect = false;

	for (FConnectType conn : AllConnectStats)
	{
		if (conn.ID == ID && conn.CanConnectID == ConnectID)
		{
			bIsCanConnect = true;
			break;
		}
		else if (conn.ID == ConnectID && conn.CanConnectID == ID)
		{
			bIsCanConnect = true;
			break;
		}
	}

	return bIsCanConnect;
}

// ������Ƭ״̬�ķ���
void AWFCMap::CacheTileStates(TArray<AWFCTile*>& Tiles)
{
	CachedTileStates.Empty(); // ���֮ǰ�Ļ�������

	for (AWFCTile* Tile : Tiles)
	{
		if (Tile)
		{
			FCachedTileState CachedState;
			CachedState.MapCoordinate = Tile->MapCoordinate;
			CachedState.CollapseState = Tile->CollapseState;
			CachedState.bIsCollapsed = Tile->bIsCollapsed;
			CachedState.MaybeStates = Tile->MaybeStates;
			CachedState.bAbleWalk = Tile->bAbleWalk;
			CachedTileStates.Add(CachedState);
		}
	}
}

// �ָ���Ƭ״̬�ķ���
void AWFCMap::RestoreTileStates(TArray<AWFCTile*>& Tiles)
{
	for (FCachedTileState& CachedState : CachedTileStates)
	{
		for (AWFCTile* Tile : Tiles)
		{
			if (Tile && Tile->MapCoordinate == CachedState.MapCoordinate)
			{
				Tile->CollapseState = CachedState.CollapseState;
				Tile->bIsCollapsed = CachedState.bIsCollapsed;
				Tile->MaybeStates = CachedState.MaybeStates;
				Tile->bAbleWalk = CachedState.bAbleWalk;
				break;
			}
		}
	}
}

bool AWFCMap::CheckConnectivity()
{
	bool flag = false;
	TArray<int32> Tag;
	Tag.AddZeroed((MapSize.X + 2) * (MapSize.Y + 2));
	TQueue<AWFCTile*> CheckTiles;
	for (AWFCTile* tile : AllTiles)
	{
		if (tile->bAbleWalk && Tag[tile->MapCoordinate.X * (MapSize.Y + 2) + tile->MapCoordinate.Y] == 0)
		{
			if (flag) return false; //�Ѿ�����һ����ͨ��
			else
			{
				flag = true;
				Tag[tile->MapCoordinate.X * (MapSize.Y + 2) + tile->MapCoordinate.Y] = 1;
				CheckTiles.Enqueue(tile);
				AWFCTile* TileNeedToCheck = nullptr;
				while (CheckTiles.Dequeue(TileNeedToCheck))
				{
					//��ȡ�ڱ���Ƭ
					FVector2D TargetCoordinates = TileNeedToCheck->MapCoordinate;

					//��
					int32 Index;
					if (TargetCoordinates.Y > 0)
					{
						Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
						//����ǿ������һ�δ�ߵ������
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					// ��
					if (TargetCoordinates.Y < MapSize.Y + 1)
					{
						Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
						//����ǿ������һ�δ�ߵ������
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					//ǰ
					if (TargetCoordinates.X > 0)
					{
						Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
						//����ǿ������һ�δ�ߵ������
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					//��
					if (TargetCoordinates.X < MapSize.X + 1)
					{
						Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
						//����ǿ������һ�δ�ߵ������
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
				}
			}
		}
	}
	return flag;
}

void AWFCMap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWFCMap::AddNavigationVolume()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	NavigationVolumeSpawnLocation = FVector(ActorLocation.X + ScaleX * MapSize.X / 2, ActorLocation.Y + ScaleY * MapSize.Y / 2, ActorLocation.Z);
	// ����NavMeshBoundsVolume
	NavVolume = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(),
		NavigationVolumeSpawnLocation,
		FRotator::ZeroRotator,
		Params
	);
	if (NavVolume)
	{
		/* ����ͨ������scale�����ɵ�������,�༭���п��������ַ�ʽ,C++������Bounds */
		//// ��������ߴ�
		//FVector VolumeSize = FVector(ScaleX * (MapSize.X + 2), ScaleY * (MapSize.Y + 2), 300.f);
		//// ���������С
		//NavVolume->SetActorScale3D(VolumeSize / 100.f); // ��Ϊ������ײ���С��100��λ

		FVector NavigationVolumeBeginLocation = FVector(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - 100.f);
		FVector NavigationVolumeEndLocation = FVector(ActorLocation.X + ScaleX * (MapSize.X + 2), ActorLocation.Y + ScaleY * (MapSize.Y + 2), ActorLocation.Z + 100.f);
		NavVolume->GetRootComponent()->Bounds = FBox(NavigationVolumeBeginLocation, NavigationVolumeEndLocation);

		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->OnNavigationBoundsAdded(NavVolume);
			NavSys->Build();
		}
	}
}

//��ͼ���ɺ��ڵ�ͼ�������
void AWFCMap::SpawnPlayer()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return;

	APawn* ExistingPawn = PlayerController->GetPawn();
	if (ExistingPawn)
	{
		PlayerController->UnPossess();
		ExistingPawn->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	//���Ե���λ�ã�ʧ����ǿ������
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode)
	{
		DefaultPawnClass = GameMode->DefaultPawnClass;
	}

	if (DefaultPawnClass)
	{
		for (AWFCTile* tile : AllTiles)
		{
			if (tile->bAbleWalk && !tile->bGrass)
			{
				TilesChooseSet.Add(tile);
			}
		}
		if (TilesChooseSet.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
			AWFCTile* SelectedTile = TilesChooseSet[RandomIndex];

			PlayerSpawnLocation = FVector(
				ActorLocation.X + ScaleX * SelectedTile->MapCoordinate.X, 
				ActorLocation.Y + ScaleY * SelectedTile->MapCoordinate.Y, 
				ActorLocation.Z + 70.f);

			TilesChooseSet.RemoveAt(RandomIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("TilesChooseSet is empty!"));
		}
		APawn* PlayerPawn = GetWorld()->SpawnActor<APawn>(
			DefaultPawnClass,
			PlayerSpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);
		if (PlayerPawn)
		{
			PlayerController->Possess(PlayerPawn);
			UE_LOG(LogTemp, Log, TEXT("Player spawned at: %s"), *PlayerSpawnLocation.ToString());
			
			ASuperNovaCharacter* SuperNovaCharacter = Cast<ASuperNovaCharacter>(PlayerPawn);
			if (SuperNovaCharacter)
			{
				if (bHardMode)
				{
					SuperNovaCharacter->EvacuationGoldNeedToPay = HardModeGoldNeedToPay;
				}
				else
				{
					SuperNovaCharacter->EvacuationGoldNeedToPay = NormalModeGoldNeedToPay;
				}
			}
		}
	}
}

//���ɿ��ܻ�ñ��ص���Ʒ
void AWFCMap::SpawnBreakableActor()
{
	int32 BreakableActorSpawnNum = (MapSize.X * MapSize.Y) / 50 + 30;
	BreakableActorSpawnNum = FMath::Min(BreakableActorSpawnNum, TilesChooseSet.Num()); //��ֹ������λ�ò���������
	UE_LOG(LogTemp, Warning, TEXT("BreakableActorSpawnNum : %d"), BreakableActorSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < BreakableActorSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//���Ե���λ�ã�ʧ����ǿ������
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (BreakableActorClasses.Num() > 0)
	{
		for (int32 i = SelectArray.Num() - 1;i >= 0;i--)
		{
			BreakableActorSpawnLocation = FVector(
				ActorLocation.X + ScaleX * TilesChooseSet[SelectArray[i]]->MapCoordinate.X,
				ActorLocation.Y + ScaleY * TilesChooseSet[SelectArray[i]]->MapCoordinate.Y,
				ActorLocation.Z + 15.f);

			int32 ChooseIndex = FMath::RandRange(0, BreakableActorClasses.Num() - 1);

			ABreakableActor* BreakableActor = GetWorld()->SpawnActor<ABreakableActor>(
				BreakableActorClasses[ChooseIndex],
				BreakableActorSpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			TilesChooseSet.RemoveAt(SelectArray[i]);
		}
	}
}

//�����λ������AI����
void AWFCMap::SpawnEnemy()
{
	int32 EnemySpawnNum = (MapSize.X * MapSize.Y) / 100 + 10;
	EnemySpawnNum = FMath::Min(EnemySpawnNum, TilesChooseSet.Num()); //��ֹ������λ�ò���������
	UE_LOG(LogTemp, Warning, TEXT("EnemySpawnNum : %d"), EnemySpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < EnemySpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//���Ե���λ�ã�ʧ����ǿ������
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (DefaultEnemyClass)
	{
		for (int32 i = SelectArray.Num() - 1;i >= 0;i--)
		{
			EnemySpawnLocation = FVector(
				ActorLocation.X + ScaleX * TilesChooseSet[SelectArray[i]]->MapCoordinate.X, 
				ActorLocation.Y + ScaleY * TilesChooseSet[SelectArray[i]]->MapCoordinate.Y, 
				ActorLocation.Z + 50.f);

			APawn* EnemyPawn = GetWorld()->SpawnActor<APawn>(
				DefaultEnemyClass,
				EnemySpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			TilesChooseSet.RemoveAt(SelectArray[i]);

			if (EnemyPawn)
			{
				UE_LOG(LogTemp, Log, TEXT("Enemy spawned at: %s"), *EnemySpawnLocation.ToString());

				//����Ѳ�ߵ�
				int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
				AWFCTile* SelectedTile = TilesChooseSet[RandomIndex];

				TargetPointSpawnLocation = FVector(
					ActorLocation.X + ScaleX * SelectedTile->MapCoordinate.X,
					ActorLocation.Y + ScaleY * SelectedTile->MapCoordinate.Y,
					ActorLocation.Z + 50.f);

				ATargetPoint* TargetPoint1 = GetWorld()->SpawnActor<ATargetPoint>(
					TargetPointSpawnLocation,
					FRotator::ZeroRotator,
					SpawnParams
				);
				ATargetPoint* TargetPoint2 = GetWorld()->SpawnActor<ATargetPoint>(
					EnemySpawnLocation, //��������ΪѲ�ߵ�֮һ
					FRotator::ZeroRotator,
					SpawnParams
				);

				AEnemy* Enemy = Cast<AEnemy>(EnemyPawn);
				if (Enemy)
				{
					Enemy->PatrolTargets.Add(TargetPoint1);
					Enemy->PatrolTargets.Add(TargetPoint2);
					Enemy->PatrolTarget = TargetPoint1;
					Enemy->MoveToTarget(TargetPoint1);
					if (bHardMode)
					{
						Enemy->EquippedWeapon->SetDamage(HardEnemyDamage);
					}
				}
			}
		}
	}
}

//���ɳ����
void AWFCMap::SpawnEvacuationPoint()
{
	int32 EvacuationPointSpawnNum = bHardMode ? 1 : 2;
	if (MapSize.X == 60 && MapSize.Y == 60) EvacuationPointSpawnNum += 1;
	EvacuationPointSpawnNum = FMath::Min(EvacuationPointSpawnNum, TilesChooseSet.Num()); //��ֹ������λ�ò���������
	UE_LOG(LogTemp, Warning, TEXT("EvacuationPointSpawnNum : %d"), EvacuationPointSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < EvacuationPointSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//���Ե���λ�ã�ʧ����ǿ������
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	DefaultEvacuationPointClass = bHardMode ? HardEvacuationPointClass : NormalEvacuationPointClass;

	if (DefaultEvacuationPointClass)
	{
		for (int32 i = SelectArray.Num() - 1;i >= 0;i--)
		{
			EvacuationPointSpawnLocation = FVector(
				ActorLocation.X + ScaleX * TilesChooseSet[SelectArray[i]]->MapCoordinate.X,
				ActorLocation.Y + ScaleY * TilesChooseSet[SelectArray[i]]->MapCoordinate.Y,
				ActorLocation.Z + 50.f);

			AEvacuationPoint* EvacuationPoint = GetWorld()->SpawnActor<AEvacuationPoint>(
				DefaultEvacuationPointClass,
				EvacuationPointSpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			TilesChooseSet.RemoveAt(SelectArray[i]);
		}
	}
}

void AWFCMap::BeginCellularAutomata()
{
	for (AWFCTile* tile : AllTiles)
	{
		AllTilesLast.Add(tile);
	}
	for (int32 t = 0; t < CellularAutomataIterations; t++)
	{
		for (AWFCTile* tile : AllTiles)
		{
			if (!tile->bAbleWalk) continue;
			int32 NumofGrass = GetNeighboringGrassTilesNumber(tile);
			if (NumofGrass == 0)
			{
				if (FMath::FRand() < ProbabilityForZero)
				{
					tile->bGrass = true;
				}
			}
			else if (NumofGrass == 1)
			{
				if (FMath::FRand() < ProbabilityForOne)
				{
					tile->bGrass = true;
				}
			}
			else if (NumofGrass == 2)
			{
				if (FMath::FRand() < ProbabilityForTwo)
				{
					tile->bGrass = true;
				}
			}
			else if (NumofGrass >= 3)
			{
				if (FMath::FRand() < ProbabilityForThreeMore)
				{
					tile->bGrass = false;
				}
			}
		}
		AllTilesLast.Empty();
		for (AWFCTile* tile : AllTiles)
		{
			AllTilesLast.Add(tile);
		}
	}
}

void AWFCMap::AddGrass()
{
	for (AWFCTile* tile : AllTiles)
	{
		if (tile->bGrass && DefaultGrassClass)
		{
			GrassPointSpawnLocation = FVector(
				ActorLocation.X + ScaleX * tile->MapCoordinate.X,
				ActorLocation.Y + ScaleY * tile->MapCoordinate.Y,
				ActorLocation.Z + 15.f);

			FActorSpawnParameters SpawnParams;
			//���Ե���λ�ã�ʧ����ǿ������
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			GetWorld()->SpawnActor<AGrass>(
				DefaultGrassClass,
				GrassPointSpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);
		}
	}
}

void AWFCMap::SpawnPortal()
{
	int32 PortalSpawnNum = MapSize.X / 10;
	PortalSpawnNum = FMath::Min(PortalSpawnNum, TilesChooseSet.Num()); //��ֹ������λ�ò���������
	UE_LOG(LogTemp, Warning, TEXT("PortalSpawnNum : %d"), PortalSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < PortalSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//���Ե���λ�ã�ʧ����ǿ������
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (DefaultPortalClass)
	{
		for (int32 i = SelectArray.Num() - 1;i >= 0;i--)
		{
			PortalSpawnLocation = FVector(
				ActorLocation.X + ScaleX * TilesChooseSet[SelectArray[i]]->MapCoordinate.X,
				ActorLocation.Y + ScaleY * TilesChooseSet[SelectArray[i]]->MapCoordinate.Y,
				ActorLocation.Z + 200.f);

			APortal* Portal = GetWorld()->SpawnActor<APortal>(
				DefaultPortalClass,
				PortalSpawnLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			AllPortals.Add(Portal);
			TilesChooseSet.RemoveAt(SelectArray[i]);
		}
		for (APortal* Portal : AllPortals)
		{
			for (APortal* OtherPortal : AllPortals)
			{
				if (Portal != OtherPortal)
				{
					Portal->PortalLocation.Add(OtherPortal->GetActorLocation());
				}
			}
		}

	}
}
