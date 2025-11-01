# -*- coding: utf-8 -*-

"""
Rhino上で EditPythonScript と入力
このコードをエディタにコピペして実行
ファイル選択ダイアログで、graph_data.txt を選択
"""

import rhinoscriptsyntax as rs
import Rhino.Geometry as rg
import scriptcontext as sc

def parse_graph_file(filepath):
    """
    C++で出力したグラフデータファイルを解析して、
    頂点リストと辺リストを返す。
    """
    vertices = []
    edges = []
    try:
        with open(filepath, 'r') as f:
            is_reading_edges = False
            for line in f:
                line = line.strip()
                if not line:
                    continue
                
                if "---EDGES---" in line:
                    is_reading_edges = True
                    continue
                
                if not is_reading_edges:
                    # 頂点座標の読み込み
                    parts = line.split()
                    x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
                    vertices.append(rg.Point3d(x, y, z))
                else:
                    # 辺の接続情報の読み込み
                    parts = line.split()
                    v1, v2 = int(parts[0]), int(parts[1])
                    edges.append((v1, v2))
        
        print("Successfully read {} vertices and {} edges.".format(len(vertices), len(edges)))
        return vertices, edges
        
    except Exception as e:
        print("Error reading or parsing file: {}".format(e))
        return None, None

def draw_graph(vertices, edges, sphere_radius=0.1, sphere_color=(255,0,0), line_color=(0,0,0)):
    """
    頂点リストと辺リストから、球と直線を描画する。
    """
    # 既存のオブジェクトを選択しないようにするため、選択を解除
    rs.UnselectAllObjects()
    
    # 描画を高速化するため、再描画を一時停止
    rs.EnableRedraw(False)
    
    # 頂点を球として描画
    sphere_ids = []
    for point in vertices:
        sphere = rs.AddSphere(point, sphere_radius)
        if sphere:
            sphere_ids.append(sphere)
    
    # 辺を直線として描画
    line_ids = []
    for v1_idx, v2_idx in edges:
        if v1_idx < len(vertices) and v2_idx < len(vertices):
            p1 = vertices[v1_idx]
            p2 = vertices[v2_idx]
            line = rs.AddLine(p1, p2)
            if line:
                line_ids.append(line)

    # オブジェクトをグループ化して管理しやすくする
    if sphere_ids or line_ids:
        all_objects = sphere_ids + line_ids
        group_name = rs.AddGroup("GraphVisualization")
        rs.AddObjectsToGroup(all_objects, group_name)
        
        # 色を設定
        rs.ObjectColor(sphere_ids, sphere_color)
        rs.ObjectColor(line_ids, line_color)

    # 再描画を有効化
    rs.EnableRedraw(True)
    print("Drawing complete.")


def main():
    """
    メイン実行関数
    """
    # ファイル選択ダイアログを表示
    filepath = rs.OpenFileName("Select Graph Data File", "Text Files (*.txt)|*.txt||")
    if not filepath:
        print("File selection cancelled.")
        return
        
    # ファイルを解析して描画
    vertices, edges = parse_graph_file(filepath)
    if vertices and edges:
        # パラメータ設定
        radius = 0.15 # 球の半径
        sphere_clr = (255, 100, 0) # 頂点の色 (オレンジ)
        line_clr = (50, 50, 50) # 辺の色 (ダークグレー)
        
        draw_graph(vertices, edges, radius, sphere_clr, line_clr)

if __name__ == "__main__":
    main()