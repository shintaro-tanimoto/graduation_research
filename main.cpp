/*
todo
- コメントの整理

How to use
g++ -o test -Iinclude main.cpp
./test graph_definitions/10.txt
dot -Tpng graph_data.dot -o graph_data.png
*/

#include "MakeBaseGraph.hpp"
#include "ExportGraph.hpp"
#include "GraphLoader.hpp"
#include <exception>

int main(int argc, char* argv[]) {
    
    // === 1. 実行時引数のチェック ===
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <definition_file.txt>" << std::endl;
        std::cerr << "Example: " << argv[0] << " cubic_lattice.txt" << std::endl;
        return 1;
    }
    std::string definition_file = argv[1];

    // === 2. ファイルからグラフ定義とルールを読み込む ===
    CoreGraph core_graph;
    std::vector<ConnectionRule> rules;

    try {
        std::cerr << "Loading definitions from " << definition_file << "..." << std::endl;
        loadDefinitions(definition_file, core_graph, rules);

    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return 1; // エラーで終了
    }

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