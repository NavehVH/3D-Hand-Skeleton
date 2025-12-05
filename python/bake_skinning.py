import pickle
import json
import numpy as np
import os

# הגדרת זוגות קבצים (קלט -> פלט)
TASKS = [
    ("assets/MANO_RIGHT.pkl", "assets/mano_right.json"),
    ("assets/MANO_LEFT.pkl", "assets/mano_left.json")
]

def normalize(v):
    norm = np.linalg.norm(v)
    if norm == 0: return v
    return v / norm

def bake_model(input_path, output_path):
    if not os.path.exists(input_path):
        print(f"Skipping {input_path} (File not found)")
        return

    print(f"Processing {input_path}...")
    with open(input_path, 'rb') as f:
        data = pickle.load(f, encoding='latin1')

    vertices = data['v_template']
    faces = data['f']
    # J_regressor מחשב את המפרקים. זה עובד זהה לשמאל ולימין.
    J_regressor = data['J_regressor']
    mano_joints = J_regressor.dot(vertices)
    mj = mano_joints
    
    # מיפוי זהה לשתי הידיים (המבנה הטופולוגי זהה)
    bones_map = {
        0: (mj[0], mj[1]),   # Wrist
        1: (mj[13], mj[14]), 2: (mj[14], mj[15]), 3: (mj[15], mj[15] + (mj[15]-mj[14])), # Thumb
        5: (mj[1], mj[2]), 6: (mj[2], mj[3]), 7: (mj[3], mj[3] + (mj[3]-mj[2])),       # Index
        9: (mj[4], mj[5]), 10: (mj[5], mj[6]), 11: (mj[6], mj[6] + (mj[6]-mj[5])),     # Middle
        13: (mj[10], mj[11]), 14: (mj[11], mj[12]), 15: (mj[12], mj[12] + (mj[12]-mj[11])), # Ring
        17: (mj[7], mj[8]), 18: (mj[8], mj[9]), 19: (mj[9], mj[9] + (mj[9]-mj[8]))     # Pinky
    }

    vertex_normals = np.zeros(vertices.shape)
    for f in faces:
        v0, v1, v2 = vertices[f[0]], vertices[f[1]], vertices[f[2]]
        face_normal = np.cross(v1 - v0, v2 - v0)
        vertex_normals[f[0]] += face_normal
        vertex_normals[f[1]] += face_normal
        vertex_normals[f[2]] += face_normal
    for i in range(len(vertex_normals)):
        vertex_normals[i] = normalize(vertex_normals[i])

    skinned_vertices = []
    
    for i, v in enumerate(vertices):
        best_id = 0
        min_dist = float('inf')
        for bid, (parent_pos, _) in bones_map.items():
            dist = np.linalg.norm(v - parent_pos)
            if dist < min_dist:
                min_dist = dist
                best_id = bid
        
        parent_pos, child_pos = bones_map[best_id]
        rest_bone_vec = child_pos - parent_pos
        rest_bone_len = np.linalg.norm(rest_bone_vec)
        rest_bone_dir = normalize(rest_bone_vec)
        
        offset = v - parent_pos
        proj_dist = np.dot(offset, rest_bone_dir)
        perp_vec = offset - proj_dist * rest_bone_dir
        normal = vertex_normals[i]

        skinned_vertices.append({
            "bid": int(best_id),
            "len": float(rest_bone_len),
            "proj": float(proj_dist),
            "px": float(perp_vec[0]), "py": float(perp_vec[1]), "pz": float(perp_vec[2]),
            "nx": float(normal[0]), "ny": float(normal[1]), "nz": float(normal[2]),
            "rvx": float(rest_bone_dir[0]), "rvy": float(rest_bone_dir[1]), "rvz": float(rest_bone_dir[2])
        })

    output_data = {"faces": faces.tolist(), "vertices": skinned_vertices}
    with open(output_path, 'w') as f:
        json.dump(output_data, f)
    print(f"Saved to {output_path}")

if __name__ == "__main__":
    for inp, outp in TASKS:
        bake_model(inp, outp)