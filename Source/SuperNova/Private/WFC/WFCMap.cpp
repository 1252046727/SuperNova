
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

	// 获取GameInstance
	USuperNovaGameInstance* GameInstance = Cast<USuperNovaGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		//UE_LOG(LogTemp, Warning, TEXT("%d:%d"), GameInstance->MapSize, GameInstance->Mode);
		if (GameInstance->MapSize == 0)//小 40 * 40
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
	/** 生成地图 **/
	InitializeMap();
	//如果有多个联通块则不符合条件重新生成
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
	/** 生成地图 **/

	//生成导航网格
	AddNavigationVolume();

	//细胞自动机算法
	BeginCellularAutomata();

	//生成陷阱
	AddGrass();

	//随机位置生成玩家
	SpawnPlayer();

	//生成可以打破的搜索容器
	SpawnBreakableActor();

	//生成AI
	SpawnEnemy();

	//生成撤离点
	SpawnEvacuationPoint();

	//生成传送门
	SpawnPortal();
}

void AWFCMap::InitializeMap()
{
	// 生成所有的 瓦片

	// 计算熵值 

	// 判断是否全部塌陷
		//获取熵值最小的瓦片
			//塌陷它并放入修改队列
			//从队列中取出瓦片直到队列为空
			// 获取他周围的瓦片
				//将不能和瓦片相接的状态删除，重新计算熵值，并加入修改队列
			//计算是否有无解
				//保存状态
				//回退

	//判断连通性
	//生成成功
	//生成失败重新生成

	//读表
	ReadTable();

	//返回当前 Actor 在世界空间（全局坐标系）中的位置
	ActorLocation = GetActorLocation();
	//返回当前 Actor 的整体缩放比例
	ActorScale = GetActorScale3D();

	ScaleX = TileSize * ActorScale.X;
	ScaleY = TileSize * ActorScale.Y;


	// 初始化所有的瓦片
	for (int32 X = 0; X <= MapSize.X + 1; X++)
	{
		for (int32 Y = 0; Y <= MapSize.Y + 1; Y++)
		{

			//计算瓦片生成位置
			//FVector SpawnLocation = FVector(ActorLocation.X + X * TileSize , ActorLocation.Y + Y * TileSize , ActorLocation.Z );

			FVector SpawnLocation = FVector(ActorLocation.X + ScaleX * X, ActorLocation.Y + ScaleY * Y, ActorLocation.Z);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AWFCTile* NewTile = GetWorld()->SpawnActor<AWFCTile>(AWFCTile::StaticClass(), SpawnLocation, GetActorRotation(), SpawnParams);
			if (NewTile)
			{
				//设置坐标
				NewTile->MapCoordinate = FVector2D(X, Y);

				//初始化瓦片的可能状态
				InitializeTileStates(NewTile);

				//添加到集合
				AllTiles.Add(NewTile);

				//UE_LOG(LogTemp, Log, TEXT("生成瓦片中.. (%d, %d)"), X, Y);

			}
		}
	}
	//UE_LOG(LogTemp, Log, TEXT("生成完毕"));

	BeginWFC();
}

void AWFCMap::ReadTable()
{
	// 设置上下文字符串，用于调试信息
	static const FString ContextString(TEXT("Fetching All Rows in Tile Stats Table"));



	if (ConnectStatsTable && TileStatsTable)
	{
		// 获取所有的连接边类型
		TArray<FConnectType*> AllConnectStatsPtr;
		ConnectStatsTable->GetAllRows(ContextString, AllConnectStatsPtr);
		for (FConnectType* Ptr : AllConnectStatsPtr)
		{
			if (Ptr)
			{
				AllConnectStats.Add(*Ptr);
			}
		}


		// 获取所有的瓦片类型
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

//初始化瓦片的可能状态
void AWFCMap::InitializeTileStates(AWFCTile* TargetTile)
{
	TargetTile->MaybeStates.Empty();
	//最外边瓦片只有空状态,保证生成的地图是封闭状态
	if (TargetTile->MapCoordinate.X == 0 || 
		TargetTile->MapCoordinate.X == MapSize.X + 1 ||
		TargetTile->MapCoordinate.Y == 0 ||
		TargetTile->MapCoordinate.Y == MapSize.Y + 1)
	{
		//附加状态
		TargetTile->MaybeStates.Add(AllTileStats[0]);
	}
	else
	{
		//设置可能的状态
		//NewTile->MaybeStates = AllTileStats;
		//每种类型的瓦片不同方向的旋转 是不同的状态
		for (FTileType type : AllTileStats)
		{

			FGameplayTag Old_L_ID = type.Connect_L_ID;
			FGameplayTag Old_R_ID = type.Connect_R_ID;
			FGameplayTag Old_F_ID = type.Connect_F_ID;
			FGameplayTag Old_B_ID = type.Connect_B_ID;

			//不同方向的状态
			TArray<FTileType> DirectionStates;
			for (int32 i : {0, 1, 2, 3})
			{
				//用于中断循环
				int32 break_i = -1;
				//判断是否为相对边 相同的物体,判断是否为4边完全相同
				if ((Old_L_ID == Old_R_ID && Old_F_ID == Old_B_ID) && Old_F_ID == Old_L_ID)
				{
					//4边完全相同 只有一种状态 不旋转
					break_i = 1;
				}
				if ((Old_L_ID == Old_R_ID && Old_F_ID == Old_B_ID) && Old_F_ID != Old_L_ID)
				{
					//对称边相同 只有两种状态 只旋转一次	
					break_i = 2;
				}
				if (i == break_i)
				{
					break;
				}


				if (i == 0)
				{
					//无旋转
					type.DirectionOfRotation = 0;
				}
				else if (i == 1)
				{
					//顺时针旋转 90
					type.DirectionOfRotation = 1;
					type.Connect_F_ID = Old_L_ID;
					type.Connect_R_ID = Old_F_ID;
					type.Connect_B_ID = Old_R_ID;
					type.Connect_L_ID = Old_B_ID;
				}
				else if (i == 2)
				{
					//旋转180
					type.DirectionOfRotation = 2;
					type.Connect_F_ID = Old_B_ID;
					type.Connect_R_ID = Old_L_ID;
					type.Connect_B_ID = Old_F_ID;
					type.Connect_L_ID = Old_R_ID;
				}
				else if (i == 3)
				{
					//旋转270
					type.DirectionOfRotation = 3;
					type.Connect_F_ID = Old_R_ID;
					type.Connect_R_ID = Old_B_ID;
					type.Connect_B_ID = Old_L_ID;
					type.Connect_L_ID = Old_F_ID;
				}
				DirectionStates.Add(type);
			}
			//附加状态
			TargetTile->MaybeStates.Append(DirectionStates);
		}
	}

	//计算熵值
	TargetTile->Entropy = CalculateEntropy(TargetTile->MaybeStates);
}

void AWFCMap::BeginWFC()
{
	//获取所有没塌陷的瓦片 
	TArray<AWFCTile*> NotCollapseTiles;
	for (AWFCTile* tile : AllTiles)
	{
		if (!tile->bIsCollapsed)
		{
			NotCollapseTiles.Add(tile);
		}
	}

	int32 NotCollapseTilesCount = NotCollapseTiles.Num();
	UE_LOG(LogTemp, Log, TEXT("没塌陷的瓦片数量: %d"), NotCollapseTilesCount);

	while (NotCollapseTilesCount != 0)
	{
		UE_LOG(LogTemp, Log, TEXT("开始计算塌陷: %d "), NotCollapseTilesCount);
		//是否退回
		bool IsRestoreTileStates = false;
		//获取最小熵值瓦片
		AWFCTile* MinEntropyTile = GetTileWithMinimumEntropy(NotCollapseTiles);
		if (MinEntropyTile == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Error: No tile found with minimum entropy."));
			break;
		}

		//保存状态, 每次塌陷之前保存当前状态
		CacheTileStates(AllTiles);

		//坍缩
		MinEntropyTile->Collapse();

		TQueue<AWFCTile*> ChangingTiles;
		ChangingTiles.Enqueue(MinEntropyTile);
		AWFCTile* TileNeedToChange = nullptr;
		while (ChangingTiles.Dequeue(TileNeedToChange))
		{
			//获取邻边瓦片
			TArray<AWFCTile*> NeighboringTiles = GetNeighboringTiles(TileNeedToChange);

			for (AWFCTile* NeighTile : NeighboringTiles)
			{
				TArray<FTileType> MaybeStates = NeighTile->MaybeStates;
				int32 ConnectCount = MaybeStates.Num();
				//TileNeedToChange已经塌陷
				if (TileNeedToChange->bIsCollapsed)
				{
					//返回塌陷瓦片相对与邻边瓦片 边的连接ID
					FGameplayTag ConnectID = TileNeedToChange->GetConnectTagToTile(NeighTile);

					//删除不能相接的状态
					for (int32 i = MaybeStates.Num() - 1; i >= 0; i--)
					{
						//返回邻边瓦片相对与塌陷瓦片 边的连接ID
						FGameplayTag IsCanConnectID = NeighTile->GetConnectTagToState(MaybeStates[i], TileNeedToChange);
						//判断是否可以相接
						if (!CanConnect(ConnectID, IsCanConnectID))
						{
							//不能相接的移除掉
							MaybeStates.RemoveAt(i);
						}
					}
				}
				//TileNeedToChange还未塌陷
				else
				{
					TArray<int32> StatesIndex; //用于记录哪些状态是可行的
					StatesIndex.Init(0, MaybeStates.Num());
					for (int32 i = TileNeedToChange->MaybeStates.Num() - 1; i >= 0; i--)
					{
						FTileType NeedToChangestate = TileNeedToChange->MaybeStates[i];
						//返回瓦片当前状态相对与邻边瓦片边的连接ID
						FGameplayTag ConnectID = TileNeedToChange->GetConnectTagToState(NeedToChangestate, NeighTile);

						//记录可以连接的状态
						for (int32 j = MaybeStates.Num() - 1; j >= 0; j--)
						{
							//返回邻边瓦片相对与塌陷瓦片边的连接ID
							FGameplayTag IsCanConnectID = NeighTile->GetConnectTagToState(MaybeStates[j], TileNeedToChange);
							//判断是否可以相接
							if (CanConnect(ConnectID, IsCanConnectID))
							{
								StatesIndex[j] = 1;
							}
						}
					}
					//删除不可行的状态
					for (int32 i = MaybeStates.Num() - 1; i >= 0; i--)
					{
						if (StatesIndex[i] == 0)
						{
							MaybeStates.RemoveAt(i);
						}
					}
				}
				//更新AllTiles中的瓦片信息
				NeighTile->MaybeStates = MaybeStates;

				if (MaybeStates.Num() == 0)
				{
					//无解 回退
					//FMapRow Cache = HistoryMap[HistoryMap.Num() - 1];
					//更新没塌陷的数量
					RestoreTileStates(AllTiles);
					//NotCollapseTilesCount++;
					//寻找保存的瓦片并退回数据
					//AllTiles = Cache.Tiles;
					IsRestoreTileStates = true;
					break;
				}

				//重新计算熵值
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
			//更新没塌陷的数量
			NotCollapseTilesCount--;

		}
		//更新没塌陷的瓦片集合 
		NotCollapseTiles.Empty();
		for (AWFCTile* tile : AllTiles)
		{
			if (!tile->bIsCollapsed)
			{
				NotCollapseTiles.Add(tile);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("完成创建"));
}

//计算权重
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

//获取最小的熵值
AWFCTile* AWFCMap::GetTileWithMinimumEntropy(TArray<AWFCTile*> Tiles)
{
	// 添加防错处理
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

	//左
	int32 Index;
	if (TargetCoordinates.Y > 0)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		//过滤已经塌陷的瓦片
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	// 右
	if (TargetCoordinates.Y < MapSize.Y + 1)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		//过滤已经塌陷的瓦片
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	//前
	if (TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		//过滤已经塌陷的瓦片
		if (!AllTiles[Index]->bIsCollapsed)
		{
			Neighs.Add(AllTiles[Index]);
		}
	}
	//后
	if (TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		//过滤已经塌陷的瓦片
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

	//左
	int32 Index;
	if (TargetCoordinates.Y > 0)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//右
	if (TargetCoordinates.Y < MapSize.Y + 1)
	{
		Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//前
	if (TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//后
	if (TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//左前
	if (TargetCoordinates.Y > 0 && TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//右前
	if (TargetCoordinates.Y < MapSize.Y + 1 && TargetCoordinates.X > 0)
	{
		Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//左后
	if (TargetCoordinates.Y > 0 && TargetCoordinates.X < MapSize.X + 1)
	{
		Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
		if (AllTilesLast[Index]->bAbleWalk && AllTilesLast[Index]->bGrass)
		{
			GrassNumber += 1;
		}
	}
	//右后
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

// 保存瓦片状态的方法
void AWFCMap::CacheTileStates(TArray<AWFCTile*>& Tiles)
{
	CachedTileStates.Empty(); // 清空之前的缓存数据

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

// 恢复瓦片状态的方法
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
			if (flag) return false; //已经存在一个连通块
			else
			{
				flag = true;
				Tag[tile->MapCoordinate.X * (MapSize.Y + 2) + tile->MapCoordinate.Y] = 1;
				CheckTiles.Enqueue(tile);
				AWFCTile* TileNeedToCheck = nullptr;
				while (CheckTiles.Dequeue(TileNeedToCheck))
				{
					//获取邻边瓦片
					FVector2D TargetCoordinates = TileNeedToCheck->MapCoordinate;

					//左
					int32 Index;
					if (TargetCoordinates.Y > 0)
					{
						Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y - 1;
						//如果是可以走且还未走的则入队
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					// 右
					if (TargetCoordinates.Y < MapSize.Y + 1)
					{
						Index = TargetCoordinates.X * (MapSize.Y + 2) + TargetCoordinates.Y + 1;
						//如果是可以走且还未走的则入队
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					//前
					if (TargetCoordinates.X > 0)
					{
						Index = (TargetCoordinates.X - 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
						//如果是可以走且还未走的则入队
						if (AllTiles[Index]->bAbleWalk && Tag[Index] == 0)
						{
							Tag[Index] = 1;
							CheckTiles.Enqueue(AllTiles[Index]);
						}
					}
					//后
					if (TargetCoordinates.X < MapSize.X + 1)
					{
						Index = (TargetCoordinates.X + 1) * (MapSize.Y + 2) + TargetCoordinates.Y;
						//如果是可以走且还未走的则入队
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
	// 创建NavMeshBoundsVolume
	NavVolume = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(),
		NavigationVolumeSpawnLocation,
		FRotator::ZeroRotator,
		Params
	);
	if (NavVolume)
	{
		/* 不能通过设置scale来生成导航网格,编辑器中可以用这种方式,C++得设置Bounds */
		//// 计算所需尺寸
		//FVector VolumeSize = FVector(ScaleX * (MapSize.X + 2), ScaleY * (MapSize.Y + 2), 300.f);
		//// 设置体积大小
		//NavVolume->SetActorScale3D(VolumeSize / 100.f); // 因为基础碰撞体大小是100单位

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

//地图生成后在地图生成玩家
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
	//尝试调整位置，失败则强制生成
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

//生成可能获得宝藏的物品
void AWFCMap::SpawnBreakableActor()
{
	int32 BreakableActorSpawnNum = (MapSize.X * MapSize.Y) / 50 + 30;
	BreakableActorSpawnNum = FMath::Min(BreakableActorSpawnNum, TilesChooseSet.Num()); //防止可生成位置不够多的情况
	UE_LOG(LogTemp, Warning, TEXT("BreakableActorSpawnNum : %d"), BreakableActorSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < BreakableActorSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//尝试调整位置，失败则强制生成
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

//在随机位置生成AI敌人
void AWFCMap::SpawnEnemy()
{
	int32 EnemySpawnNum = (MapSize.X * MapSize.Y) / 100 + 10;
	EnemySpawnNum = FMath::Min(EnemySpawnNum, TilesChooseSet.Num()); //防止可生成位置不够多的情况
	UE_LOG(LogTemp, Warning, TEXT("EnemySpawnNum : %d"), EnemySpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < EnemySpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//尝试调整位置，失败则强制生成
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

				//生成巡逻点
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
					EnemySpawnLocation, //出生点作为巡逻点之一
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

//生成撤离点
void AWFCMap::SpawnEvacuationPoint()
{
	int32 EvacuationPointSpawnNum = bHardMode ? 1 : 2;
	if (MapSize.X == 60 && MapSize.Y == 60) EvacuationPointSpawnNum += 1;
	EvacuationPointSpawnNum = FMath::Min(EvacuationPointSpawnNum, TilesChooseSet.Num()); //防止可生成位置不够多的情况
	UE_LOG(LogTemp, Warning, TEXT("EvacuationPointSpawnNum : %d"), EvacuationPointSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < EvacuationPointSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//尝试调整位置，失败则强制生成
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
			//尝试调整位置，失败则强制生成
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
	PortalSpawnNum = FMath::Min(PortalSpawnNum, TilesChooseSet.Num()); //防止可生成位置不够多的情况
	UE_LOG(LogTemp, Warning, TEXT("PortalSpawnNum : %d"), PortalSpawnNum);

	TArray<int32> SelectArray;
	while (SelectArray.Num() < PortalSpawnNum)
	{
		int32 RandomIndex = FMath::RandRange(0, TilesChooseSet.Num() - 1);
		SelectArray.AddUnique(RandomIndex);
	}
	SelectArray.Sort();

	FActorSpawnParameters SpawnParams;
	//尝试调整位置，失败则强制生成
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
