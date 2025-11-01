#ifndef CONSTRAINED_SEARCH_HPP
#define CONSTRAINED_SEARCH_HPP

#include <set>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <queue> // BFSのために追加
#include <iostream>
#include <stdexcept>
#include "1_core_graph/MakeBaseGraph.hpp" 
#include "tdzdd/util/Graph.hpp" 

// ヘルパー: 頂点名 (例: "0_a") からベースタイプ (例: "a") を抽出
inline std::string getBaseType(const std::string& full_name) {
    size_t underscore = full_name.find('_');
    if (underscore == std::string::npos) return ""; 
    return full_name.substr(underscore + 1);
}

// ヘルパー: G' (0_a 以外の 'a' タイプを除外した) グラフを構築
inline std::map<std::string, std::set<std::string>> buildFilteredAdjacencyList(
    tdzdd::Graph& original_graph, 
    const std::string& root_name
) {
    std::map<std::string, std::set<std::string>> adj_list;
    std::set<std::string> valid_vertices; 
    std::string root_type = getBaseType(root_name);

    if (root_type.empty()) {
        throw std::runtime_error("Root name is invalid (no type): " + root_name);
    }

    // G' に含まれるべき頂点を定義
    for (int i = 1; i <= original_graph.vertexSize(); ++i) {
        std::string v_name = original_graph.vertexName(i);
        std::string v_type = getBaseType(v_name);
        
        // root_type (例: 'a') と同じタイプだが、root_name (例: '0_a') 自身ではない頂点は除外
        if (v_type == root_type && v_name != root_name) {
            continue; 
        }
        valid_vertices.insert(v_name);
    }

    // G' の隣接リストを構築
    for (int i = 0; i < original_graph.edgeSize(); ++i) {
        const auto& edge = original_graph.edgeInfo(i);
        std::string u_name = original_graph.vertexName(edge.v1);
        std::string v_name = original_graph.vertexName(edge.v2);

        // 両方の頂点が G' に含まれるエッジのみを追加
        if (valid_vertices.count(u_name) && valid_vertices.count(v_name)) {
            adj_list[u_name].insert(v_name);
            adj_list[v_name].insert(u_name);
        }
    }
    return adj_list;
}

// 【新設】 ヘルパー: BFSで始点からのホップ数を計算
inline std::map<std::string, int> calculateDistancesBFS(
    const std::string& start_node,
    const std::map<std::string, std::set<std::string>>& adj_list
) {
    std::map<std::string, int> distances;
    std::queue<std::pair<std::string, int>> q;

    if (adj_list.count(start_node) == 0) {
        return distances; // 始点がグラフに存在しない
    }

    q.push({start_node, 0});
    distances[start_node] = 0;

    while (!q.empty()) {
        std::string current_node = q.front().first;
        int current_dist = q.front().second;
        q.pop();

        // adj_list.at() は .count() チェック済みの想定
        if (adj_list.count(current_node) == 0) continue; 

        for (const std::string& neighbor : adj_list.at(current_node)) {
            // 未訪問 (distances にキーがない) なら距離を記録してキューに追加
            if (distances.find(neighbor) == distances.end()) {
                distances[neighbor] = current_dist + 1;
                q.push({neighbor, current_dist + 1});
            }
        }
    }
    return distances;
}

// 【新設】 ヘルパー: G' を G'' にフィルタリング (n-1 ホップ以内)
inline std::map<std::string, std::set<std::string>> filterGraphByDistance(
    const std::map<std::string, std::set<std::string>>& adj_list_G_prime,
    const std::map<std::string, int>& distances,
    int max_distance // (n-1) が渡される
) {
    std::map<std::string, std::set<std::string>> adj_list_G_double_prime;
    std::set<std::string> valid_nodes_in_G_double_prime;

    // 1. max_distance (n-1) 以内の頂点をすべて特定
    for (const auto& pair : distances) {
        if (pair.second <= max_distance) {
            valid_nodes_in_G_double_prime.insert(pair.first);
        }
    }

    // 2. G' (adj_list_G_prime) を走査し、G'' にエッジを追加
    for (const auto& pair : adj_list_G_prime) {
        const std::string& u = pair.first;
        
        // そもそも u が G'' に含まれない頂点ならスキップ
        if (valid_nodes_in_G_double_prime.count(u) == 0) continue;

        // u は G'' に含まれる。隣接頂点 v をチェック
        for (const std::string& v : pair.second) {
            // v も G'' に含まれるなら、G'' の隣接リストに追加
            if (valid_nodes_in_G_double_prime.count(v)) {
                adj_list_G_double_prime[u].insert(v);
            }
        }
    }
    
    return adj_list_G_double_prime;
}


/**
 * @brief 再帰的なバックトラッキング関数 (アルゴリズムのコア)
 * (変更なし)
 */
void findSolutionsRecursive(
    std::set<std::string>& current_path,            
    std::set<std::string>& types_collected,         
    const std::set<std::string>& frontier,          
    std::set<std::set<std::string>>& all_solutions, // (set に変更済み)
    const std::map<std::string, std::set<std::string>>& adj_list, 
    const std::set<std::string>& all_types          
) {
    // 1. 成功のベースケース
    if (types_collected.size() == all_types.size()) {
        all_solutions.insert(current_path); // (insert に変更済み)
        return; 
    }

    // 2. 失敗のベースケース
    if (frontier.empty()) {
        return; 
    }

    // 3. 再帰ステップ
    for (const std::string& next_vertex : frontier) {
        
        std::string v_type = getBaseType(next_vertex);

        // すでに収集済みのタイプは無視
        if (v_type.empty() || types_collected.count(v_type)) {
            continue;
        }
        
        // (A) 選択
        current_path.insert(next_vertex);
        types_collected.insert(v_type);

        // (B) 新しいフロンティアを計算
        std::set<std::string> new_frontier = frontier;
        new_frontier.erase(next_vertex); 

        if (adj_list.count(next_vertex)) { 
            const auto& new_neighbors = adj_list.at(next_vertex);
            for (const std::string& n : new_neighbors) {
                if (current_path.count(n) == 0) { // まだパスに含まれていない隣人
                    new_frontier.insert(n);
                }
            }
        }
        
        // (C) フロンティアをフィルタリング (収集済みタイプはフロンティアから除外)
        std::set<std::string> filtered_frontier;
        for (const std::string& v_f : new_frontier) {
            std::string f_type = getBaseType(v_f);
            if (f_type.empty() || !types_collected.count(f_type)) {
                filtered_frontier.insert(v_f);
            }
        }

        // (D) 再帰
        findSolutionsRecursive(current_path, types_collected, filtered_frontier, all_solutions, adj_list, all_types);

        // (E) バックトラック
        current_path.erase(next_vertex);
        types_collected.erase(v_type);
    }
}

/**
 * @brief 制約付きグラフ列挙のメイン関数
 * (BFS事前枝刈り＋詳細デバッグ出力)
 */
std::set<std::set<std::string>> findAllConstrainedGraphs(
    tdzdd::Graph& graph, 
    const CoreGraph& core_graph, 
    const std::string& root_name,
    std::ostream& log_stream // <-- 【追加】
) {
    std::set<std::set<std::string>> all_solutions; 

    // 1. コアタイプ数 n を取得
    std::set<std::string> all_types;
    for (int i = 1; i <= core_graph.vertexSize(); ++i) {
        try { all_types.insert(core_graph.vertexName(i)); } catch (...) {}
    }
    if (all_types.empty()) {
         // 致命的な警告は cerr にも出す
         std::cerr << "Warning: No types found in core graph." << std::endl;
         log_stream << "Warning: No types found in core graph." << std::endl;
         return all_solutions;
    }
    int num_types = all_types.size();
    int max_distance = num_types - 1; 
    log_stream << "  Core types found (num_types=" << num_types << "). Max hop distance set to " << max_distance << "." << std::endl;


    // 2. G' (隣接リスト) を作成
    log_stream << "  Building G' (filtered adjacency list)..." << std::endl;
    std::map<std::string, std::set<std::string>> adj_list_G_prime = buildFilteredAdjacencyList(graph, root_name); 

    if (adj_list_G_prime.count(root_name) == 0) {
         // 致命的な警告は cerr にも出す
         std::cerr << "Warning: Root vertex " << root_name << " is not in the filtered graph (or has no edges)." << std::endl;
         log_stream << "Warning: Root vertex " << root_name << " is not in the filtered graph (or has no edges)." << std::endl;
         return all_solutions;
    }

    // 3. G' でBFSを実行し、ホップ数を計算
    log_stream << "  Running BFS from " << root_name << " on G'..." << std::endl;
    std::map<std::string, int> distances = calculateDistancesBFS(root_name, adj_list_G_prime); 

    // 4. G'' (n-1 ホップ以内) を作成
    log_stream << "  Building G'' (filtering by max distance)..." << std::endl;

    // --- デバッグ: G' の全頂点セットを作成 ---
    std::set<std::string> all_nodes_in_g_prime;
    for(const auto& pair : adj_list_G_prime) {
        all_nodes_in_g_prime.insert(pair.first);
        for(const std::string& v : pair.second) {
            all_nodes_in_g_prime.insert(v);
        }
    }
    int g_prime_total_nodes = all_nodes_in_g_prime.size();

    // G'' を作成
    std::map<std::string, std::set<std::string>> adj_list_G_double_prime = 
        filterGraphByDistance(adj_list_G_prime, distances, max_distance); 

    // --- デバッグ: G'' の全頂点セットを作成 ---
    std::set<std::string> all_nodes_in_g_double_prime;
    for(const auto& pair : adj_list_G_double_prime) {
        all_nodes_in_g_double_prime.insert(pair.first);
        for(const std::string& v : pair.second) {
            all_nodes_in_g_double_prime.insert(v);
        }
    }
    int g_double_prime_total_nodes = all_nodes_in_g_double_prime.size();

    // --- デバッグ出力 (log_stream へ) ---
    log_stream << "  [DEBUG] G' (original) total vertices: " << g_prime_total_nodes << std::endl;
    log_stream << "  [DEBUG] G' vertices list: (";
    bool first_g_prime = true;
    for (const std::string& v : all_nodes_in_g_prime) {
        if (!first_g_prime) {
            log_stream << ", ";
        }
        log_stream << v;
        first_g_prime = false;
    }
    if (first_g_prime) {
        log_stream << "None";
    }
    log_stream << ")" << std::endl;

    log_stream << "  [DEBUG] G'' (pruned) total vertices: " << g_double_prime_total_nodes << std::endl;
    log_stream << "  [DEBUG] Vertices pruned by BFS: " << (g_prime_total_nodes - g_double_prime_total_nodes) << std::endl;

    log_stream << "  [DEBUG] Pruned vertices list: (";
    bool first_pruned = true;
    for (const std::string& v : all_nodes_in_g_prime) {
        if (all_nodes_in_g_double_prime.count(v) == 0) {
            if (!first_pruned) {
                log_stream << ", ";
            }
            log_stream << v; 
            first_pruned = false;
        }
    }
    if (first_pruned) {
        log_stream << "None";
    }
    log_stream << ")" << std::endl;
    // --- デバッグここまで ---


    if (adj_list_G_double_prime.count(root_name) == 0) {
         std::cerr << "Warning: Root vertex " << root_name << " was pruned by BFS (or has no edges in G'')." << std::endl;
         log_stream << "Warning: Root vertex " << root_name << " was pruned by BFS (or has no edges in G'')." << std::endl;
         return all_solutions;
    }

    // 5. G'' を使ってバックトラッキング探索を開始
    std::set<std::string> initial_path = {root_name};
    std::set<std::string> types_collected = {getBaseType(root_name)};
    std::set<std::string> initial_frontier_raw = adj_list_G_double_prime.at(root_name);
    
    std::set<std::string> initial_frontier;
     for (const std::string& v_f : initial_frontier_raw) {
        std::string f_type = getBaseType(v_f);
        if (f_type.empty() || !types_collected.count(f_type)) {
            initial_frontier.insert(v_f);
        }
    }

    log_stream << "  Starting recursive search on G''..." << std::endl;
    findSolutionsRecursive(initial_path, types_collected, initial_frontier, 
                           all_solutions, adj_list_G_double_prime, all_types); 

    return all_solutions;
}

#endif // CONSTRAINED_SEARCH_HPP