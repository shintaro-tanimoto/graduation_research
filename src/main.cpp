#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <exception>
#include <set>
#include <map>
#include <algorithm> 
#include <iomanip>   
#include <sstream>     
#include <filesystem> 

// --- 必要なプロジェクトヘッダ ---
#include "1_core_graph/GraphLoader.hpp"
#include "2_search/ConstrainedSearch.hpp"
#include "3_geometry/SolutionMesh.hpp"
#include "3_geometry/DualGraph.hpp"
#include "9_export/ExportGraph.hpp" 
#include "4_analysis/GraphIsomorphism.hpp" // <-- 【追加】 nauty のため

// --- ソート用ヘルパー (変更なし) ---
auto compare_vertices = [](const std::string& s1, const std::string& s2) {
    size_t sep1 = s1.find('_');
    std::string type1 = s1.substr(sep1 + 1); 
    int id1 = std::stoi(s1.substr(0, sep1)); 
    size_t sep2 = s2.find('_');
    std::string type2 = s2.substr(sep2 + 1); 
    int id2 = std::stoi(s2.substr(0, sep2)); 
    if (type1 != type2) {
        return type1 < type2; 
    }
    return id1 < id2; 
};
auto compare_solutions = [](const std::set<std::string>& solA, const std::set<std::string>& solB) { 
    std::vector<std::string> vecA(solA.begin(), solA.end());
    std::vector<std::string> vecB(solB.begin(), solB.end());
    std::sort(vecA.begin(), vecA.end(), compare_vertices);
    std::sort(vecB.begin(), vecB.end(), compare_vertices);
    return vecA < vecB; 
};
// --- メイン関数 ---

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <definition_file.txt>" << std::endl;
        return 1;
    }
    std::string definition_file = argv[1];

    std::string basename;
    try {
        std::filesystem::path p(definition_file);
        basename = p.stem().string(); 
    } catch (const std::exception& e) {
        basename = "output";
    }
    std::string output_dir = "output/" + basename + "/"; 
    std::string output_prefix = output_dir + basename + "_"; // "output/4/4_"

    CoreGraph core_graph;
    std::vector<ConnectionRule> rules;
    std::map<std::string, ObjMesh> mesh_data;
    GraphData base_data; 
    std::set<std::set<std::string>> solutions;

    try {
        try {
            std::filesystem::create_directories(output_dir);
        } catch (const std::exception& e) {
            std::cerr << "Error: Could not create output directory: " << output_dir << std::endl;
            std::cerr << e.what() << std::endl;
            return 1;
        }

        std::cerr << "Loading definitions from " << definition_file << "..." << std::endl;
        loadDefinitions(definition_file, core_graph, rules, mesh_data);

        std::string log_filename = output_dir + "generation_log.txt";
        std::ofstream log_file(log_filename);
        std::cerr << "Verbose logs will be written to " << log_filename << std::endl;
        
        int num_types = core_graph.vertexSize(); 
        int n = std::max(1, num_types - 1); 
        std::cerr << "Generating a base graph (num_types=" << num_types << ", n=" << n << ")..." << std::endl;
        
        base_data = make_base_graph(core_graph, rules, n, log_file);
        
        exportCoreConnectivityForRhino(base_data, output_prefix + "core_graph_data.txt", std::cerr);
        exportFullGraphForChecking(base_data.full_graph, output_prefix + "graph_data.dot", std::cerr);

        std::cerr << "Enumerating constrained graphs via backtracking..." << std::endl;
        std::string root_vertex = "0_a"; 
        
        solutions = findAllConstrainedGraphs(base_data.full_graph, core_graph, root_vertex, log_file);

        std::cerr << "Found " << solutions.size() << " total graphs matching the constraints." << std::endl;
        
        log_file.close(); 

    } catch (const std::exception& e) {
        std::cerr << "Initialization or Search failed: " << e.what() << std::endl;
        return 1; 
    }

    // --- ▼ 【修正】 nauty 組み込み ▼ ---
    try {
        // (ソートはログを見やすくするためにも実行)
        std::vector<std::set<std::string>> sorted_solutions(solutions.begin(), solutions.end());
        std::sort(sorted_solutions.begin(), sorted_solutions.end(), compare_solutions);

        std::ofstream sol_file(output_dir + "constrained_solutions.txt");
        std::cerr << "Writing solutions to " << output_dir << "constrained_solutions.txt" << std::endl;

        std::ofstream log_file(output_dir + "generation_log.txt", std::ios_base::app); 
        
        // --- 1. 全解の双対グラフをまず生成する ---
        std::cerr << "Building dual graphs for all " << sorted_solutions.size() << " solutions..." << std::endl;
        std::vector<tdzdd::Graph> all_dual_graphs;
        // (ソート済み解のリストを保存しておく)
        std::vector<std::set<std::string>> all_sorted_solutions_vec;

        for (const auto& solution_set : sorted_solutions) {
            // (ログは log_file に出力)
            ObjMesh solution_mesh = buildSolutionMesh(
                solution_set, base_data, mesh_data, log_file 
            );
            tdzdd::Graph dual_graph = buildDualGraph(solution_mesh);
            
            all_dual_graphs.push_back(dual_graph);
            all_sorted_solutions_vec.push_back(solution_set);
        }

        // --- 2. Nauty で同型性判定・フィルタリング ---
        std::cerr << "Filtering unique graphs via Nauty..." << std::endl;
        // (filterUniqueGraphsNauty は map<string, tdzdd::Graph> を返す)
        // (キー: 正規ラベル, 値: 代表グラフ)
        std::map<std::string, tdzdd::Graph> unique_dual_graphs = filterUniqueGraphsNauty(all_dual_graphs);

        std::cerr << "Found " << unique_dual_graphs.size() << " unique (non-isomorphic) graphs." << std::endl;

        // --- 3. ユニークなグラフ（の代表解）のみ OBJ/DOT 出力 ---
        std::cerr << "Writing OBJ/DOT files for unique graphs..." << std::endl;

        // (キーである「正規ラベル」-> 代表解の「セット」 をマッピングする)
        std::map<std::string, std::set<std::string>> canonical_to_solution_set;
        for(size_t i = 0; i < all_dual_graphs.size(); ++i) {
            std::string key = getCanonicalLabel(all_dual_graphs[i]);
            if (canonical_to_solution_set.count(key) == 0) {
                // このキーがまだ登録されていなければ、
                // この解 (i) を「代表解」として登録
                canonical_to_solution_set[key] = all_sorted_solutions_vec[i];
            }
        }

        int unique_idx = 0;
        // (unique_graphs マップをループ)
        for (const auto& pair : unique_dual_graphs) {
            std::string key = pair.first;
            const tdzdd::Graph& representative_dual_graph = pair.second;
            
            // このキー (正規ラベル) に対応する「代表解」セットを取得
            const std::set<std::string>& representative_solution_set = canonical_to_solution_set.at(key);

            // 代表解からファイル名を生成
            std::vector<std::string> sorted_vertices(representative_solution_set.begin(), representative_solution_set.end());
            std::sort(sorted_vertices.begin(), sorted_vertices.end(), compare_vertices);
            std::stringstream ss_name;
            for (size_t i = 0; i < sorted_vertices.size(); ++i) {
                if (i > 0) ss_name << "_";
                ss_name << sorted_vertices[i];
            }
            std::string solution_name_part = ss_name.str();
            
            // 解リスト (txt) に追記 (Unique 代表)
            sol_file << "--- Unique Graph " << unique_idx << " (Representative: " << solution_name_part << ") ---" << std::endl;
            
            // OBJ と DOT を出力
            std::string obj_filename = output_prefix + "UNIQUE_" + std::to_string(unique_idx) + "_" + solution_name_part + ".obj";
            std::string dot_filename = output_prefix + "UNIQUE_" + std::to_string(unique_idx) + "_" + solution_name_part + "_dual_graph.dot";

            // (OBJのメッシュは再計算する必要がある)
            log_file << "  Building UNIQUE mesh " << unique_idx << ": " << obj_filename << "..." << std::endl;
            ObjMesh solution_mesh = buildSolutionMesh(
                representative_solution_set, base_data, mesh_data, log_file 
            );
            exportObjMesh(solution_mesh, obj_filename, log_file); 
            
            // (DOTは計算済みのものを出力)
            log_file << "  Building UNIQUE dual graph " << unique_idx << ": " << dot_filename << "..." << std::endl;
            exportFullGraphForChecking(representative_dual_graph, dot_filename, log_file); 

            unique_idx++;
        }
        
        sol_file.close();
        log_file.close(); 

    } catch (const std::exception& e) {
        std::cerr << "Error during solution processing or export: " << e.what() << std::endl;
        return 1;
    }
    // --- ▲ 【修正】 ▲ ---

    std::cerr << "All processing complete." << std::endl;
    return 0;
}