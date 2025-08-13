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

// ��Ƭ״̬��Ϣ�Ľṹ��
struct FCachedTileState
{
	FVector2D MapCoordinate;
	FTileType CollapseState;
	bool bIsCollapsed;
	bool bAbleWalk;
	TArray<FTileType> MaybeStates;
};

/**
	������̮�� ��ͼ
*/
UCLASS()
class SUPERNOVA_API AWFCMap : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWFCMap();

	// ��ͼ��С
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	FVector2D MapSize;

	// ��Ƭ���
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	float TileSize;

	//���е���Ƭ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> AllTiles;

	//��һ��ϸ���Զ����������е���Ƭ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> AllTilesLast;

	//��ʼ����Ƭ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<AWFCTile*> InitTiles;

	//���ӱ����͵ı��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	UDataTable* ConnectStatsTable;

	//��Ƭ���ͱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	UDataTable* TileStatsTable;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCMap")
	TArray<FMapRow> HistoryMap;*/

	// ����������Ƭ״̬������
	TArray<FCachedTileState> CachedTileStates;

	//���е���������
	TArray<FConnectType> AllConnectStats;

	//���е���Ƭ����
	TArray<FTileType> AllTileStats;

	//��ͼ�������֮���������Actor����Ƭ
	TArray<AWFCTile*> TilesChooseSet;

	// ��ʼ��
	void InitializeMap();

	//����
	void ReadTable();

	//��ʼ��������Ƭ�Ŀ���״̬
	void InitializeTileStates(AWFCTile* TargetTile);

	//��ʼ̮��
	void BeginWFC();

	//������ֵ
	float CalculateEntropy(TArray<FTileType>);

	//��ȡ��ֵ��С����Ƭ
	AWFCTile* GetTileWithMinimumEntropy(TArray<AWFCTile*> Tiles);

	//��ȡ���ڵ���Ƭ
	TArray<AWFCTile*> GetNeighboringTiles(AWFCTile* TargetTile);

	//��ȡ���ڰ˸��������ƬΪ���������
	int32 GetNeighboringGrassTilesNumber(AWFCTile* TargetTile);

	//�ж��Ƿ�������
	bool CanConnect(FGameplayTag ID, FGameplayTag ConnectID);

	// ������Ƭ״̬�ķ���
	void CacheTileStates(TArray<AWFCTile*>& AllTiles);

	// �ָ���Ƭ״̬�ķ���
	void RestoreTileStates(TArray<AWFCTile*>& AllTiles);

	//�����ͨ��
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

	bool bHardMode; //�Ƿ�Ϊ����ģʽ

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

	//��Χ�˸�����û�����壬��Ϊ����ĸ���
	UPROPERTY(EditAnywhere)
	float  ProbabilityForZero = 0.08f;

	//��Χ�˸�������һ�����壬��Ϊ����ĸ���
	UPROPERTY(EditAnywhere)
	float  ProbabilityForOne = 0.2f;

	//��Χ�˸������ж������壬��Ϊ����ĸ���
	UPROPERTY(EditAnywhere)
	float  ProbabilityForTwo = 0.4f;

	//��Χ�˸������г����������壬��Ϊ������ĸ���
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

	//��ʼϸ���Զ����㷨
	void BeginCellularAutomata();

	//��������
	void AddGrass();

	//���ɵ�������
	void AddNavigationVolume();

	//�������
	void SpawnPlayer();

	//���ɿ��Դ��Ƶ���������
	void SpawnBreakableActor();
	
	//����AI����
	void SpawnEnemy();

	//���ɳ����
	void SpawnEvacuationPoint();

	//���ɴ�����
	void SpawnPortal();
};
