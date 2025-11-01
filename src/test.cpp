#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <iomanip> // std::setprecision のために必要

// プロジェクトヘッダ
#include "1_core_graph/MakeBaseGraph.hpp"
#include "1_core_graph/GraphLoader.hpp"
#include "3_geometry/ObjTypes.hpp"
#include "9_export/ExportGraph.hpp"      // exportObjMesh を使うため
#include "3_geometry/VertexMesh.hpp"    // getMeshForVertex をテストするため
#include "3_geometry/SolutionMesh.hpp"   // exportSolutionMesh をテストするため
#include "3_geometry/DualGraph.hpp"     // buildDualGraph をテストするため
#include "4_analysis/GraphIsomorphism.hpp" // Nautyラッパーのテスト

/**
 * @brief GraphLoader, MakeBaseGraph, VertexMesh の動作をテストします。
 */
int main(int argc, char* argv[]) {
/*
    CoreGraph core_graph;
    std::vector<ConnectionRule> rules;
    std::map<std::string, ObjMesh> mesh_data;
    GraphData base_data; 

    // 4.txt をハードコードして使用
    std::string definition_file = "graph_definitions/4.txt";
    std::cerr << "Loading test definitions from " << definition_file << "..." << std::endl;

    try {
        // 1. GraphLoader のテスト (メッシュ読み込み)
        loadDefinitions(definition_file, core_graph, rules, mesh_data);

        std::cerr << "  Loaded " << mesh_data.size() << " mesh definitions." << std::endl;
        if (mesh_data.count("a")) {
            std::cerr << "    Type 'a' mesh loaded: " << mesh_data.at("a").vertices.size() 
                      << " vertices, " << mesh_data.at("a").faces.size() << " faces." << std::endl;
        }

        // 2. MakeBaseGraph のテスト (n=1 で実行)
        // (4.txt は num_types=2 なので、main.cpp の動的ロジックでも n=1 になります)
        int n = 1; 
        std::cerr << "Generating a base graph with n = " << n << "..." << std::endl;
        base_data = make_base_graph(core_graph, rules, n);

    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return 1; 
    }

    std::cerr << "--- Debugging VertexMesh ---" << std::endl;

    // 3. getMeshForVertex ("0_a") のテスト
    try {
        ObjMesh mesh_0_a = getMeshForVertex("0_a", base_data, mesh_data);
        exportObjMesh(mesh_0_a, "output/debug_0_a.obj");
    } catch (const std::exception& e) {
        std::cerr << "Failed to export 0_a: " << e.what() << std::endl;
    }

    // 4. getMeshForVertex ("1_b") のテスト
    // (4.txt, n=1 で 1_b が存在するはず)
    try {
        ObjMesh mesh_1_b = getMeshForVertex("1_b", base_data, mesh_data);
        exportObjMesh(mesh_1_b, "output/debug_1_b.obj");
    } catch (const std::exception& e) {
        std::cerr << "Failed to export 1_b: " << e.what() << std::endl;
    }
    std::cerr << "----------------------------" << std::endl;

    std::cerr << "Test complete. Check 'output/debug_0_a.obj' and 'output/debug_1_b.obj'." << std::endl;

    // 5. SolutionMesh.hpp のテスト
    std::cerr << "--- Debugging SolutionMesh ---" << std::endl;

    // "0_a" と "1_b" を含む解を定義
    // (4.txt の n=1 で、0_a と 1_b は CONNECT a b で接しているはず)
    std::set<std::string> test_solution = {"0_a", "1_b"};

    // 2つのメッシュを統合し、接合面を削除して出力
    try {
        exportSolutionMesh(
            test_solution, 
            "output/debug_solution_0a_1b.obj", // 出力ファイル名
            base_data, 
            mesh_data
        );
    } catch (const std::exception& e) {
        std::cerr << "Failed to export solution mesh: " << e.what() << std::endl;
    }
    std::cerr << "------------------------------" << std::endl;

    std::cerr << "Test complete. Check 'output/debug_solution_0a_1b.obj'." << std::endl;

    // 6. DualGraph.hpp のテスト
    std::cerr << "--- Debugging DualGraph ---" << std::endl;
    try {
        // (test_solution = {"0_a", "1_b"} は上で定義済み)
        
        // 6a. SolutionMesh からメッシュを *ビルド* する
        ObjMesh solution_mesh = buildSolutionMesh(
            test_solution,
            base_data,
            mesh_data
        );
        
        // 6b. ビルドしたメッシュから双対グラフを生成
        tdzdd::Graph dual_graph = buildDualGraph(solution_mesh);
        
        std::cerr << "  Dual graph created." << std::endl;
        std::cerr << "    Nodes (Faces): " << dual_graph.vertexSize() << std::endl;
        std::cerr << "    Edges (Shared Edges): " << dual_graph.edgeSize() << std::endl;
        
        // --- ▼ 【追加】 双対グラフを.dotファイルに出力 ▼ ---
        exportFullGraphForChecking(dual_graph, "output/debug_dual_graph.dot");
        std::cerr << "  Dual graph DOT file was written to output/debug_dual_graph.dot" << std::endl;
        // --- ▲ 【追加】 ▲ ---
            
    } catch (const std::exception& e) {
        std::cerr << "Failed to build dual graph: " << e.what() << std::endl;
    }
    std::cerr << "---------------------------" << std::endl;

    std::cerr << "Test complete." << std::endl;
*/

// 7. GraphIsomorphism.hpp (nauty) のテスト
    std::cerr << "--- Debugging GraphIsomorphism (Nauty) ---" << std::endl;

    // G1: 4頂点・4辺 の「四角形」
    tdzdd::Graph g1;
    g1.addEdge("0", "1");
    g1.addEdge("1", "2");
    g1.addEdge("2", "3");
    g1.addEdge("3", "0");
    g1.update();

    // G2: G1と「同型」なグラフ (ラベル名だけが違う)
    tdzdd::Graph g2;
    g2.addEdge("a", "b"); // <-- nautyラッパーが 0 にマッピング
    g2.addEdge("b", "c"); // <-- 1 にマッピング
    g2.addEdge("c", "d"); // <-- 2 にマッピング
    g2.addEdge("d", "a"); // <-- 3 にマッピング
    g2.update();

    // G3: G1と「同型でない」グラフ (辺の数が違う)
    tdzdd::Graph g3;
    g3.addEdge("0", "1");
    g3.addEdge("1", "2");
    g3.addEdge("2", "3");
    g3.update(); // 辺は3つ

    std::vector<tdzdd::Graph> test_graphs = {g1, g2, g3};

    // フィルターを実行
    std::map<std::string, tdzdd::Graph> unique_set = filterUniqueGraphsNauty(test_graphs);

    std::cerr << "  Input graphs: 3 (g1, g2, g3)" << std::endl;
    std::cerr << "  Unique graphs found (via Nauty): " << unique_set.size() << std::endl;

    if (unique_set.size() == 2) {
        std::cerr << "  Test PASSED." << std::endl;
    } else {
        std::cerr << "  Test FAILED. (Expected 2)" << std::endl;
    }
    std::cerr << "--------------------------------------" << std::endl;

    std::cerr << "Test complete." << std::endl;

    return 0;
}