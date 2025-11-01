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

/**
 * @brief GraphLoader, MakeBaseGraph, VertexMesh の動作をテストします。
 */
int main(int argc, char* argv[]) {

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

    return 0;
}