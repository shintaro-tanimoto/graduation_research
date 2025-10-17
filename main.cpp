#include "MakeBaseGraph.hpp"
#include "ExportGraph.hpp" // ▼▼▼ 新しいヘッダファイルをインクルード ▼▼▼

int main() {
    // === 1. コアグラフの定義 ===
    CoreGraph core_graph;
    core_graph.addEdge("a", "b");
    core_graph.addEdge("b", "c");
    core_graph.update();

    // === 2. 接続ルールの定義 ===
    std::vector<ConnectionRule> rules = {
        {{1, 0, 0}, {{"a", "a"}, {"c", "b"}}},
        {{-1, 0, 0}, {{"a", "a"}, {"b", "c"}}},
        {{0, 1, 0}, {{"a", "b"}, {"a", "c"}}},
        {{0, -1, 0}, {{"b", "a"}, {"c", "a"}}},
        {{0, 0, 1}, {{"a", "a"}, {"b", "b"}, {"c", "c"}}},
        {{0, 0, -1}, {{"a", "a"}, {"b", "b"}, {"c", "c"}}}
    };

    // === 3. グラフ生成の実行 ===
    int n = 2; // 成長ステップ数
    std::cerr << "Generating a base graph with n = " << n << "..." << std::endl;
    GraphData result_data = make_base_graph(core_graph, rules, n);

    // === 4. 結果を2種類のファイルに出力 ===
    
    // 4a. Rhino用にコアの骨格グラフを出力
    exportCoreConnectivityForRhino(result_data, "core_graph_data.txt");
    
    // 4b. 内容確認用に全体の詳細なグラフを出力
    exportFullGraphForChecking(result_data.full_graph, "graph_data.dot");

    return 0;
}