#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WFCTile.h"
#include "WFCMap.generated.h"

//USTRUCT(BlueprintType)
//struct FMapRow
//{
//	GENERATED_BODY()
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
//	TArray<AWFCTile *> Tiles;
//};

// 瓦片状态信息的结构体
struct FCachedTileState
{
	FVector2D MapCoordinate;
	FTileType CollapseState;
	bool bIsCollapsed;
	bool bAbleWalk;
	TArray<FTileType> MaybeStates;
};

/**
	波函数坍缩 地图
*/
UCLASS()
class SUPERNOVA_API AWFCMap : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWFCMap();

	// 地图大小
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	FVector2D MapSize;

	// 瓦片宽度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	float TileSize;

	//所有的瓦片
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> AllTiles;

	//上一次细胞自动机迭代所有的瓦片
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> AllTilesLast;

	//初始的瓦片
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> InitTiles;

	//连接边类型的表格
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	UDataTable* ConnectStatsTable;

	//瓦片类型表格
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	UDataTable* TileStatsTable;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<FMapRow> HistoryMap;*/

	// 保存所有瓦片状态的数组
	TArray<FCachedTileState> CachedTileStates;

	//所有的连接类型
	TArray<FConnectType> AllConnectStats;

	//所有的瓦片类型
	TArray<FTileType> AllTileStats;

	//地图生成完毕之后可以生成Actor的瓦片
	TArray<AWFCTile*> TilesChooseSet;

	// 初始化
	void InitializeMap();

	//读表
	void ReadTable();

	//初始化设置瓦片的可能状态
	void InitializeTileStates(AWFCTile* TargetTile);

	//开始坍陷
	void BeginWFC();

	//计算熵值
	float CalculateEntropy(TArray<FTileType>);

	//获取熵值最小的瓦片
	AWFCTile* GetTileWithMinimumEntropy(TArray<AWFCTile*> Tiles);

	//获取相邻的瓦片
	TArray<AWFCTile*> GetNeighboringTiles(AWFCTile* TargetTile);

	//获取相邻八个方向的瓦片为陷阱的数量
	int32 GetNeighboringGrassTilesNumber(AWFCTile* TargetTile);

	//判断是否可以相接
	bool CanConnect(FGameplayTag ID, FGameplayTag ConnectID);

	// 保存瓦片状态的方法
	void CacheTileStates(TArray<AWFCTile*>& AllTiles);

	// 恢复瓦片状态的方法
	void RestoreTileStates(TArray<AWFCTile*>& AllTiles);

	//检查联通性
	bool CheckConnectivity();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UClass* DefaultPawnClass;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TSubclassOf<class AEnemy> DefaultEnemyClass;

	UPROPERTY()
	FVector PlayerSpawnLocation;

	UPROPERTY()
	FVector BreakableActorSpawnLocation;

	UPROPERTY()
	FVector EnemySpawnLocation;

	UPROPERTY()
	FVector TargetPointSpawnLocation;

	UPROPERTY()
	FVector ActorLocation;

	UPROPERTY()
	FVector ActorScale;

	UPROPERTY()
	float ScaleX;

	UPROPERTY()
	float ScaleY;

	UPROPERTY()
	class ANavMeshBoundsVolume* NavVolume;

	UPROPERTY()
	FVector NavigationVolumeSpawnLocation;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TArray<TSubclassOf<class ABreakableActor>> BreakableActorClasses;

	bool bHardMode; //是否为困难模式

	UPROPERTY(EditAnywhere)
	float HardEnemyDamage;

	UPROPERTY(EditAnywhere)
	int32 HardModeGoldNeedToPay;

	UPROPERTY(EditAnywhere)
	int32 NormalModeGoldNeedToPay;

	TSubclassOf<class AEvacuationPoint> DefaultEvacuationPointClass;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TSubclassOf<class AEvacuationPoint> NormalEvacuationPointClass;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TSubclassOf<class AEvacuationPoint> HardEvacuationPointClass;

	UPROPERTY()
	FVector EvacuationPointSpawnLocation;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TSubclassOf<class AGrass> DefaultGrassClass;

	UPROPERTY()
	FVector GrassPointSpawnLocation;

	UPROPERTY(EditAnywhere)
	int32  CellularAutomataIterations = 5;

	//周围八个网格没有陷阱，变为陷阱的概率
	UPROPERTY(EditAnywhere)
	float  ProbabilityForZero = 0.08f;

	//周围八个网格有一个陷阱，变为陷阱的概率
	UPROPERTY(EditAnywhere)
	float  ProbabilityForOne = 0.2f;

	//周围八个网格有二个陷阱，变为陷阱的概率
	UPROPERTY(EditAnywhere)
	float  ProbabilityForTwo = 0.4f;

	//周围八个网格有超过三个陷阱，变为非陷阱的概率
	UPROPERTY(EditAnywhere)
	float  ProbabilityForThreeMore = 0.8f;

	UPROPERTY(EditAnywhere, Category = SpawnClass)
	TSubclassOf<class APortal> DefaultPortalClass;

	UPROPERTY()
	FVector PortalSpawnLocation;

	TArray<APortal*> AllPortals;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//开始细胞自动机算法
	void BeginCellularAutomata();

	//生成陷阱
	void AddGrass();

	//生成导航网格
	void AddNavigationVolume();

	//生成玩家
	void SpawnPlayer();

	//生成可以打破的搜索容器
	void SpawnBreakableActor();
	
	//生成AI敌人
	void SpawnEnemy();

	//生成撤离点
	void SpawnEvacuationPoint();

	//生成传送门
	void SpawnPortal();
};
