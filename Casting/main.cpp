#include "stdafx.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Common/SDK/Types.h"
#include "Common/Geometry/Dcel/Tools.h"
#include "Common/Optimization/Lp/IncrementalLp.h"
#include "Casting.h"

using namespace Common;
using namespace std;

vector<SDK::Point3<double>> verticesObj;
vector<int> trianglesObj;

void read() {
    ifstream infile("../Assets/cube_1x1x1.obj");
    assert(!infile.fail());

    string line;
    while (getline(infile, line))
    {
        istringstream iss(line);
        string start;
        iss >> start;
        if (start == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            verticesObj.emplace_back(-z, y, x);
        }
        else if (start == "f") {
            int a, b, c;
            iss >> a >> b >> c;
            trianglesObj.push_back(a - 1);
            trianglesObj.push_back(b - 1);
            trianglesObj.push_back(c - 1);
        }
    }

    for (size_t i = 0; i < trianglesObj.size() / 3; ++i) {
        auto c = SDK::Cross(verticesObj[trianglesObj[i * 3 + 1]] - verticesObj[trianglesObj[i * 3]], verticesObj[trianglesObj[i * 3 + 2]] - verticesObj[trianglesObj[i * 3]]);
        assert(c.LenSq() > 1e-12);
    }
}

int main()
{
    read();

    const int verticesCount = (int)verticesObj.size();
    const int trianglesCount = (int)trianglesObj.size() / 3;

    unordered_set<pair<int, int>, Common::pairhash> edges;
    for (int t = 0; t < trianglesCount; ++t) {
        for (int i = 0; i < 3; ++i) {
            const int vStart = trianglesObj[t * 3 + i];
            const int vEnd = trianglesObj[t * 3 + (i + 1) % 3];
            if (edges.find(make_pair(vEnd, vStart)) == edges.end()) {
                edges.insert(make_pair(vStart, vEnd));
            }
        }
    }
    const auto m = Dcel::Create(trianglesObj.cbegin(), trianglesObj.cend(), verticesCount, (int)edges.size());

    const auto bigFaces = Casting::CombineFaces(m, verticesObj);

    vector<int> candidates(12);
    vector<SDK::HalfPlane<double>> matrix;
    vector<int> indices;
    for (int i = 2; i < Casting::kCoverHemispheresCount; ++i) {
        matrix.clear();
        indices.clear();
        const auto actualNormal = Casting::hemisphereCoverForSphere[i];
        assert(!SDK::AlmostEqualToZero(Cross(Casting::upperHemisphereDirection, actualNormal).LenSq(), kMaxDiffSq));
        const auto rotation = SDK::RotationBetween(Normalize(actualNormal), Casting::upperHemisphereDirection, kMaxDiffSq);
        const auto& n = Casting::hemisphereCoverForSphere[i];
        for (int j = 0; j < (int)bigFaces.size(); ++j) {
            const auto faceNormal = bigFaces[j].GetNormal();
            const double cos = Dot(faceNormal, n);
            if (cos >= -kMaxDiff * 100) {
                const auto rotatedFaceNormal = rotation * faceNormal;
                assert(SDK::AlmostEqualRelativeAndAbs(Dot(rotatedFaceNormal, Casting::upperHemisphereDirection), cos, kMaxDiff));
                const auto halfPlane = Casting::ProjectIntersectionOnUpperHemispherePlane(rotatedFaceNormal);
                matrix.push_back(halfPlane);
                indices.push_back(j);
            }
        }

        const auto coverIndices = Casting::FindCover(matrix);
        for (int ci : coverIndices) {
            candidates.push_back(indices[ci]);
        }
    }

    for (const int candidateIndex : candidates) {
        bigFaces[candidateIndex].GetNormal();
    }

    return 0;
}


