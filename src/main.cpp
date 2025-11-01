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
#include <filesystem> // create_directory のため

// --- 必要なプロジェクトヘッダ ---
#include "1_core_graph/GraphLoader.hpp"
#include "2_search/ConstrainedSearch.hpp"
#include "3_geometry/SolutionMesh.hpp"
#include "3_geometry/DualGraph.hpp"
#include "9_export/ExportGraph.hpp" 

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
        std::cerr << "Warning: Could not parse filepath. Using 'output' as prefix. " << e.what() << std::endl;
        basename = "output";
    }

    // --- ▼ 【修正】 出力ディレクトリとプレフィックスの定義 ▼ ---
    std::string output_dir = "output/" + basename + "/"; 
    std::string output_prefix = output_dir + basename + "_"; // "output/4/4_"
    // --- ▲ 【修正】 ▲ ---

    CoreGraph core_graph;
    std::vector<ConnectionRule> rules;
    std::map<std::string, ObjMesh> mesh_data;
    GraphData base_data; 
    std::set<std::set<std::string>> solutions;

    try {
        // --- ▼ 【修正】 出力ディレクトリを作成 ▼ ---
        try {
            std::filesystem::create_directories(output_dir);
        } catch (const std::exception& e) {
            std::cerr << "Error: Could not create output directory: " << output_dir << std::endl;
            std::cerr << e.what() << std::endl;
            return 1;
        }
        // --- ▲ 【修正】 ▲ ---

        std::cerr << "Loading definitions from " << definition_file << "..." << std::endl;
        loadDefinitions(definition_file, core_graph, rules, mesh_data);

        std::string log_filename = output_dir + "generation_log.txt"; // (プレフィックスなし)
        std::ofstream log_file(log_filename);
        std::cerr << "Verbose logs will be written to " << log_filename << std::endl;
        
        int num_types = core_graph.vertexSize(); 
        int n = std::max(1, num_types - 1); 
        std::cerr << "Generating a base graph (num_types=" << num_types << ", n=" << n << ")..." << std::endl;
        
        base_data = make_base_graph(core_graph, rules, n, log_file);
        
        // (出力先ファイル名が output_prefix を使うように変更)
        exportCoreConnectivityForRhino(base_data, output_prefix + "core_graph_data.txt", std::cerr);
        exportFullGraphForChecking(base_data.full_graph, output_prefix + "graph_data.dot", std::cerr);

        std::cerr << "Enumerating constrained graphs via backtracking..." << std::endl;
        std::string root_vertex = "0_a"; 
        
        solutions = findAllConstrainedGraphs(base_data.full_graph, core_graph, root_vertex, log_file);

        std::cerr << "Found " << solutions.size() << " graphs matching the constraints." << std::endl;
        
        log_file.close(); 

    } catch (const std::exception& e) {
        std::cerr << "Initialization or Search failed: " << e.what() << std::endl;
        return 1; 
    }

    try {
        std::vector<std::set<std::string>> sorted_solutions(solutions.begin(), solutions.end());
        std::sort(sorted_solutions.begin(), sorted_solutions.end(), compare_solutions);

        // (出力先ファイル名が output_dir を使うように変更)
        std::ofstream sol_file(output_dir + "constrained_solutions.txt");
        std::cerr << "Writing solutions to " << output_dir << "constrained_solutions.txt" << std::endl;

        std::ofstream log_file(output_dir + "generation_log.txt", std::ios_base::app); 

        if (!sorted_solutions.empty()) {
            std::cerr << "Building mesh 0-" << (sorted_solutions.size() - 1) 
                      << ": " << output_dir << "..." << std::endl; // (プレフィックス -> ディレクトリ)
        }

        int sol_idx = 0;
        for (const auto& solution_set : sorted_solutions) {
            
            std::vector<std::string> sorted_vertices(solution_set.begin(), solution_set.end());
            std::sort(sorted_vertices.begin(), sorted_vertices.end(), compare_vertices);
            std::stringstream ss_name;
            for (size_t i = 0; i < sorted_vertices.size(); ++i) {
                if (i > 0) ss_name << "_";
                ss_name << sorted_vertices[i];
            }
            std::string solution_name_part = ss_name.str();
            
            sol_file << "Solution " << sol_idx << ": (" << solution_name_part << ")" << std::endl;

            // (出力先ファイル名が output_prefix を使うように変更)
            std::string obj_filename = output_prefix + solution_name_part + ".obj";
            log_file << "  Building mesh " << sol_idx << ": " << obj_filename << "..." << std::endl;
            
            ObjMesh solution_mesh = buildSolutionMesh(
                solution_set,
                base_data,
                mesh_data,
                log_file 
            );
            exportObjMesh(solution_mesh, obj_filename, log_file); 

            // (出力先ファイル名が output_prefix を使うように変更)
            std::string dot_filename = output_prefix + solution_name_part + "_dual_graph.dot";
            log_file << "  Building dual graph " << sol_idx << ": " << dot_filename << "..." << std::endl;
            tdzdd::Graph dual_graph = buildDualGraph(solution_mesh);
            exportFullGraphForChecking(dual_graph, dot_filename, log_file); 

            sol_idx++;
        }
        
        sol_file.close();
        log_file.close(); 

    } catch (const std::exception& e) {
        std::cerr << "Error during solution processing or export: " << e.what() << std::endl;
        return 1;
    }

    std::cerr << "All processing complete." << std::endl;
    return 0;
}