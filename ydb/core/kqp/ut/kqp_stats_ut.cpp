#include <ydb/core/kqp/ut/common/kqp_ut_common.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/public/sdk/cpp/client/resources/ydb_resources.h>

#include <ydb/library/yql/dq/actors/compute/dq_compute_actor.h>

#include <cstdlib>

namespace NKikimr {
namespace NKqp {

using namespace NYdb;
using namespace NYdb::NTable;

Y_UNIT_TEST_SUITE(KqpStats) {

Y_UNIT_TEST(MultiTxStatsFullExp) {
    TKikimrRunner kikimr;
    NExperimental::TStreamQueryClient db{kikimr.GetDriver()};
    auto settings = NExperimental::TExecuteStreamQuerySettings();
    settings.ProfileMode(NYdb::NExperimental::EStreamQueryProfileMode::Full);

    auto it = db.ExecuteStreamQuery(R"(
        SELECT * FROM `/Root/EightShard` WHERE Key BETWEEN 150 AND 266 ORDER BY Data LIMIT 4;
    )", settings).GetValueSync();

    auto res = CollectStreamResult(it);
    UNIT_ASSERT_C(it.IsSuccess(), it.GetIssues().ToString());
    UNIT_ASSERT_VALUES_EQUAL(res.ResultSetYson, R"([[[1];[202u];["Value2"]];[[2];[201u];["Value1"]];[[3];[203u];["Value3"]]])");

    UNIT_ASSERT(res.PlanJson);
    NJson::TJsonValue plan;
    NJson::ReadJsonTree(*res.PlanJson, &plan, true);
    auto node = FindPlanNodeByKv(plan, "Node Type", "TopSort-TableRangeScan");
    UNIT_ASSERT_EQUAL(node.GetMap().at("Stats").GetMapSafe().at("TotalTasks").GetIntegerSafe(), 2);
}

Y_UNIT_TEST(JoinNoStats) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    TStreamExecScanQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::None);

    auto it = db.StreamExecuteScanQuery(R"(
        SELECT count(*) FROM `/Root/EightShard` AS t JOIN `/Root/KeyValue` AS kv ON t.Data = kv.Key;
    )", settings).GetValueSync();

    auto res = CollectStreamResult(it);
    UNIT_ASSERT_C(it.IsSuccess(), it.GetIssues().ToString());
    UNIT_ASSERT_VALUES_EQUAL(res.ResultSetYson, "[[16u]]");

    UNIT_ASSERT(!res.QueryStats);
    UNIT_ASSERT(!res.PlanJson);
}

Y_UNIT_TEST(JoinStatsBasic) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    TStreamExecScanQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Basic);

    auto it = db.StreamExecuteScanQuery(R"(
        SELECT count(*) FROM `/Root/EightShard` AS t JOIN `/Root/KeyValue` AS kv ON t.Data = kv.Key;
    )", settings).GetValueSync();

    auto res = CollectStreamResult(it);
    UNIT_ASSERT_C(it.IsSuccess(), it.GetIssues().ToString());

    UNIT_ASSERT_VALUES_EQUAL(res.ResultSetYson, "[[16u]]");

    UNIT_ASSERT(res.QueryStats);
    UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases().size(), 2);
    if (res.QueryStats->query_phases(0).table_access(0).name() == "/Root/KeyValue") {
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).name(), "/Root/KeyValue");
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).partitions_count(), 1);
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(1).name(), "/Root/EightShard");
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(1).partitions_count(), 8);
    } else {
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).name(), "/Root/EightShard");
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).partitions_count(), 8);
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(1).name(), "/Root/KeyValue");
        UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(1).partitions_count(), 1);
    }

    UNIT_ASSERT(!res.PlanJson);
}

Y_UNIT_TEST(MultiTxStatsFull) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    TStreamExecScanQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Full);

    auto it = db.StreamExecuteScanQuery(R"(
        SELECT * FROM `/Root/EightShard` WHERE Key BETWEEN 150 AND 266 ORDER BY Data LIMIT 4;
    )", settings).GetValueSync();

    auto res = CollectStreamResult(it);
    UNIT_ASSERT_C(it.IsSuccess(), it.GetIssues().ToString());
    UNIT_ASSERT_VALUES_EQUAL(
        res.ResultSetYson,
        R"([[[1];[202u];["Value2"]];[[2];[201u];["Value1"]];[[3];[203u];["Value3"]]])"
    );

    UNIT_ASSERT(res.QueryStats);
    UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases().size(), 1);
    UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).name(), "/Root/EightShard");
    UNIT_ASSERT_VALUES_EQUAL(res.QueryStats->query_phases(0).table_access(0).partitions_count(), 2);

    UNIT_ASSERT(res.PlanJson);
    NJson::TJsonValue plan;
    NJson::ReadJsonTree(*res.PlanJson, &plan, true);
    auto node = FindPlanNodeByKv(plan, "Node Type", "TopSort-TableRangeScan");
    UNIT_ASSERT_EQUAL(node.GetMap().at("Stats").GetMapSafe().at("TotalTasks").GetIntegerSafe(), 2);
}

Y_UNIT_TEST(DeferredEffects) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();
    TString planJson;
    NJson::TJsonValue plan;

    TExecDataQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Full);

    auto result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "true";

        UPSERT INTO `/Root/TwoShard`
        SELECT Key + 100u AS Key, Value1 FROM `/Root/TwoShard` WHERE Key in (1,2,3,4,5);
    )", TTxControl::BeginTx(), settings).ExtractValueSync();
    UNIT_ASSERT_VALUES_EQUAL_C(result.GetStatus(), EStatus::SUCCESS, result.GetIssues().ToString());

    // TODO(sk): do proper phase dependency tracking
    //
    // NJson::ReadJsonTree(result.GetQueryPlan(), &plan, true);
    // auto node = FindPlanNodeByKv(plan, "Node Type", "TablePointLookup");
    // UNIT_ASSERT_EQUAL(node.GetMap().at("Stats").GetMapSafe().at("TotalTasks").GetIntegerSafe(), 1);

    auto tx = result.GetTransaction();
    UNIT_ASSERT(tx);

    auto params = db.GetParamsBuilder()
        .AddParam("$key")
            .Uint32(100)
            .Build()
        .AddParam("$value")
            .String("New")
            .Build()
        .Build();

    result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "true";

        DECLARE $key AS Uint32;
        DECLARE $value AS String;

        UPSERT INTO `/Root/TwoShard` (Key, Value1) VALUES
            ($key, $value);
    )", TTxControl::Tx(*tx).CommitTx(), std::move(params), settings).ExtractValueSync();
    UNIT_ASSERT_VALUES_EQUAL_C(result.GetStatus(), EStatus::SUCCESS, result.GetIssues().ToString());

    NJson::ReadJsonTree(result.GetQueryPlan(), &plan, true);
    UNIT_ASSERT_VALUES_EQUAL(plan.GetMapSafe().at("Plan").GetMapSafe().at("Plans").GetArraySafe().size(), 3);

    result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "true";
        SELECT * FROM `/Root/TwoShard`;
        UPDATE `/Root/TwoShard` SET Value1 = "XXX" WHERE Key in (3,600);
    )", TTxControl::BeginTx().CommitTx(), settings).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);
    UNIT_ASSERT_VALUES_EQUAL_C(result.GetStatus(), EStatus::SUCCESS, result.GetIssues().ToString());

    NJson::ReadJsonTree(result.GetQueryPlan(), &plan, true);
    UNIT_ASSERT_VALUES_EQUAL(plan.GetMapSafe().at("Plan").GetMapSafe().at("Plans").GetArraySafe().size(), 3);

    auto ru = result.GetResponseMetadata().find(NYdb::YDB_CONSUMED_UNITS_HEADER);
    UNIT_ASSERT(ru != result.GetResponseMetadata().end());

    UNIT_ASSERT(std::atoi(ru->second.c_str()) > 1);
}

Y_UNIT_TEST(DataQueryWithEffects) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    TExecDataQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Full);

    auto result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "true";

        UPSERT INTO `/Root/TwoShard`
        SELECT Key + 1u AS Key, Value1 FROM `/Root/TwoShard`;
    )", TTxControl::BeginTx(TTxSettings::SerializableRW()).CommitTx(), settings).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);
    AssertSuccessResult(result);

    NJson::TJsonValue plan;
    NJson::ReadJsonTree(result.GetQueryPlan(), &plan, true);

    auto node = FindPlanNodeByKv(plan, "Node Type", "Upsert-ConstantExpr");
    UNIT_ASSERT_EQUAL(node.GetMap().at("Stats").GetMapSafe().at("TotalTasks").GetIntegerSafe(), 2);
}

Y_UNIT_TEST(DataQueryOldEngine) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    TExecDataQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Full);

    auto result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "false";

        UPSERT INTO `/Root/TwoShard`
        SELECT Key + 1u AS Key, Value1 FROM `/Root/TwoShard`;
    )", TTxControl::BeginTx(TTxSettings::SerializableRW()).CommitTx(), settings).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);
    AssertSuccessResult(result);
}

Y_UNIT_TEST(DataQueryMulti) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    TExecDataQuerySettings settings;
    settings.CollectQueryStats(ECollectQueryStatsMode::Full);

    auto result = session.ExecuteDataQuery(R"(
        PRAGMA kikimr.UseNewEngine = "true";

        SELECT 1;
        SELECT 2;
        SELECT 3;
        SELECT 4;
    )", TTxControl::BeginTx(TTxSettings::SerializableRW()).CommitTx(), settings).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);
    AssertSuccessResult(result);

    NJson::TJsonValue plan;
    NJson::ReadJsonTree(result.GetQueryPlan(), &plan, true);
    UNIT_ASSERT_EQUAL_C(plan.GetMapSafe().at("Plan").GetMapSafe().at("Plans").GetArraySafe().size(), 0, result.GetQueryPlan());
}

Y_UNIT_TEST_NEW_ENGINE(RequestUnitForBadRequestExecute) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    auto result = session.ExecuteDataQuery(Q_(R"(
            INCORRECT_STMT
        )"), TTxControl::BeginTx(TTxSettings::SerializableRW()).CommitTx(), TExecDataQuerySettings().ReportCostInfo(true))
        .ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);

    auto ru = result.GetResponseMetadata().find(NYdb::YDB_CONSUMED_UNITS_HEADER);
    UNIT_ASSERT(ru != result.GetResponseMetadata().end());
    UNIT_ASSERT_VALUES_EQUAL(ru->second, "1");
    UNIT_ASSERT_VALUES_EQUAL(result.GetStatus(), EStatus::GENERIC_ERROR);
    UNIT_ASSERT(result.GetConsumedRu() > 0);
}

Y_UNIT_TEST_NEW_ENGINE(RequestUnitForBadRequestExplicitPrepare) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    auto result = session.PrepareDataQuery(Q_(R"(
        INCORRECT_STMT
    )"), TPrepareDataQuerySettings().ReportCostInfo(true)).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);

    auto ru = result.GetResponseMetadata().find(NYdb::YDB_CONSUMED_UNITS_HEADER);
    UNIT_ASSERT(ru != result.GetResponseMetadata().end());
    UNIT_ASSERT_VALUES_EQUAL(ru->second, "1");
    UNIT_ASSERT_VALUES_EQUAL(result.GetStatus(), EStatus::GENERIC_ERROR);
    UNIT_ASSERT(result.GetConsumedRu() > 0);
}

Y_UNIT_TEST_NEW_ENGINE(RequestUnitForSuccessExplicitPrepare) {
    TKikimrRunner kikimr;
    auto db = kikimr.GetTableClient();
    auto session = db.CreateSession().GetValueSync().GetSession();

    auto result = session.PrepareDataQuery(Q_(R"(
        SELECT 0; SELECT 1; SELECT 2; SELECT 3; SELECT 4;
        SELECT 5; SELECT 6; SELECT 7; SELECT 8; SELECT 9;
    )"), TPrepareDataQuerySettings().ReportCostInfo(true)).ExtractValueSync();
    result.GetIssues().PrintTo(Cerr);

    auto ru = result.GetResponseMetadata().find(NYdb::YDB_CONSUMED_UNITS_HEADER);
    UNIT_ASSERT(ru != result.GetResponseMetadata().end());
    UNIT_ASSERT(atoi(ru->second.c_str()) > 1);
    UNIT_ASSERT_VALUES_EQUAL(result.GetStatus(), EStatus::SUCCESS);
    UNIT_ASSERT(result.GetConsumedRu() > 1);
}

} // suite

} // namespace NKqp
} // namespace NKikimr
