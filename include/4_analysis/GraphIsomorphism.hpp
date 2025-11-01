#ifndef GRAPH_ISOMORPHISM_HPP
#define GRAPH_ISOMORPHISM_HPP

#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <sstream> // std::stringstream
#include <cstring> // C言語のメモリ操作 (malloc) のため
#include <algorithm> // std::sort

#include "tdzdd/util/Graph.hpp"

// C++コードから C言語の nauty ヘッダをインクルードする
extern "C" {
    #include "nauty.h" 
    #include "nausparse.h"  // (sparsegraph, sparsenauty の定義)
    #include "naututil.h"   // (SG_ALLOC, SG_FREE, sortlists_sg の定義)
}

/**
 * @brief tdzdd::Graph を nauty が要求する sparsegraph (sg) 形式に変換します。
 * (Segmentation Fault 対策で、2パスアルゴリズムに完全書き換え)
 */
inline void convertToSparseGraph(
    const tdzdd::Graph& g,
    sparsegraph& sg,
    std::vector<int>& lab,
    std::vector<int>& ptn,
    std::vector<int>& orbits,
    std::map<std::string, int>& vertex_to_int
) {
    int v_count_tdzdd = g.vertexSize();
    int e_count = g.edgeSize();
    
    // --- パス 1: C++ の隣接リストと頂点マップを作成 ---
    int next_int_id = 0;
    // (nauty の頂点ID -> nauty の隣接リスト)
    std::map<int, std::vector<int>> adj_list_int; 

    for (int i = 0; i < e_count; ++i) {
        const auto& edge = g.edgeInfo(i);
        std::string name1 = g.vertexName(edge.v1);
        std::string name2 = g.vertexName(edge.v2);

        // 頂点名 ("0", "1"...) を nauty の ID (0, 1...) にマッピング
        if (vertex_to_int.count(name1) == 0) vertex_to_int[name1] = next_int_id++;
        if (vertex_to_int.count(name2) == 0) vertex_to_int[name2] = next_int_id++;

        int u = vertex_to_int[name1];
        int v = vertex_to_int[name2];
        
        // C++の隣接リストに (u, v) と (v, u) の両方を追加
        adj_list_int[u].push_back(v);
        adj_list_int[v].push_back(u);
    }
    
    // 孤立点もマッピングに追加
    for (int v_tdzdd = 1; v_tdzdd <= v_count_tdzdd; ++v_tdzdd) {
         std::string name = g.vertexName(v_tdzdd);
         if (vertex_to_int.count(name) == 0) {
             vertex_to_int[name] = next_int_id++;
         }
    }

    int v_count = vertex_to_int.size();
    if (v_count == 0) {
        SG_INIT(sg);
        return;
    }

    lab.resize(v_count);
    ptn.resize(v_count);
    orbits.resize(v_count);

    // --- パス 2: nauty の sparsegraph 構造体を正しく構築 ---
    SG_INIT(sg);
    SG_ALLOC(sg, v_count, e_count * 2, "malloc");
    sg.nv = v_count; 
    sg.nde = e_count * 2;

    int k = 0; // sg.e 配列用のグローバルインデックス
    
    // v_count (nauty の頂点数 0 から n-1) でループ
    for (int i = 0; i < v_count; ++i) {
        sg.v[i] = k; // 頂点 i の隣接リストは sg.e[k] から始まる
        
        if (adj_list_int.count(i)) { 
            // 頂点 i に隣接する頂点 (neighbor) が存在する場合
            auto& neighbors = adj_list_int.at(i);
            
            // nauty の要求仕様: 隣接リストはソートされている必要がある
            std::sort(neighbors.begin(), neighbors.end());
            
            sg.d[i] = neighbors.size(); // 頂点 i の次数
            
            // ソート済みの隣接リストを sg.e にコピー
            for (int neighbor : neighbors) {
                sg.e[k] = neighbor;
                k++;
            }
        } else {
            // 孤立点の場合
            sg.d[i] = 0;
        }
    }
    // (この時点で k == e_count * 2 となるはず)
}


/**
 * @brief nauty を呼び出し、グラフの正規形 (Canonical Label) を文字列として取得します。
 * (変更なし)
 */
inline std::string getCanonicalLabel(const tdzdd::Graph& g) {
    
    sparsegraph sg;
    std::vector<int> lab, ptn, orbits;
    std::map<std::string, int> vertex_to_int;

    // 1. TdZddグラフを nauty の sparsegraph 形式に変換
    convertToSparseGraph(g, sg, lab, ptn, orbits, vertex_to_int);

    int v_count = sg.nv;
    if (v_count == 0) {
        SG_FREE(sg); // 空でも SG_ALLOC が呼ばれている可能性があるので解放
        return "empty";
    }

    sparsegraph canong;
    SG_INIT(canong);

    static DEFAULTOPTIONS_SPARSEGRAPH(options);
    options.getcanon = TRUE; 
    options.writeautoms = FALSE; 
    
    statsblk stats; 

    // 2. nauty (sparsenauty) の呼び出し
    sparsenauty(&sg, lab.data(), ptn.data(), orbits.data(), &options, &stats, &canong);

    // 3. 正規グラフ (canong) を一意な文字列に変換
    std::stringstream ss;
    ss << "v:" << canong.nv << " e:" << (canong.nde / 2) << " edges:";
    
    // sparsenauty が生成した canong はソート済みとは限らないため、
    // 一意な文字列を生成するためにソートする
    sortlists_sg(&canong); 
    
    for (int i = 0; i < canong.nv; ++i) {
        ss << " " << i << ":[";
        std::vector<int> neighbors;
        for (int j = 0; j < canong.d[i]; ++j) {
            neighbors.push_back(canong.e[canong.v[i] + j]);
        }
        std::sort(neighbors.begin(), neighbors.end()); 
        
        for (size_t j = 0; j < neighbors.size(); ++j) {
            ss << neighbors[j] << (j == neighbors.size() - 1 ? "" : ",");
        }
        ss << "]";
    }
    
    // 4. メモリ解放
    SG_FREE(sg); 
    SG_FREE(canong);

    return ss.str(); 
}


/**
 * @brief 双対グラフのリストを受け取り、nauty の正規形に基づいてユニークなグラフを抽出します。
 * (変更なし)
 */
inline std::map<std::string, tdzdd::Graph> filterUniqueGraphsNauty(
    const std::vector<tdzdd::Graph>& all_graphs
) {
    std::map<std::string, tdzdd::Graph> unique_graphs;

    for (const auto& g : all_graphs) {
        std::string key = getCanonicalLabel(g);
        
        if (unique_graphs.count(key) == 0) {
            unique_graphs[key] = g;
        }
    }

    return unique_graphs;
}

#endif // GRAPH_ISOMORPHISM_HPP