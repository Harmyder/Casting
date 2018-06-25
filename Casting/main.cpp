#include "stdafx.h"

#include <fstream>
#include <sstream>
#include <vector>
#include "Common/SDK/Types.h"
#include "Common/Geometry/Dcel/Tools.h"
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

array<int, 3> FindCoverage(const vector<SDK::Plane<double>>& planes, const SDK::Point3<double>& actualNormal) {
    const SDK::Point3<double> normal(0., 0., 1.);
    assert(!SDK::AlmostEqualToZero(Cross(normal, actualNormal).LenSq(), maxDiffSq));
    const auto rotation = SDK::RotationBetween(normal, Normalize(actualNormal), maxDiffSq);

    for (const auto& plane : planes) {
        rotation * plane.GetNormal();
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

    const auto bigFaces = CombineFaces(m, verticesObj);

    // covering set for $\mathcal{S}^2$
    const array<SDK::Point3<double>, 4> hemisphereCoverForSphere = {
        SDK::Normalize(SDK::Point3<double>( 1.,  .1,  0.)),
        SDK::Normalize(SDK::Point3<double>(-1.,  .1,  0.)),
        // small z-axis aligned cleft left uncovered toward negaitve y-axis 
        SDK::Normalize(SDK::Point3<double>( 0., -.1,  1.)),
        SDK::Normalize(SDK::Point3<double>( 0., -.1, -1.))
    };

    vector<SDK::Plane<double>> matrix;
    for (int i = 0; i < 4; ++i) {
        matrix.clear();
        copy_if(bigFaces.cbegin(), bigFaces.cend(), back_inserter(matrix), [n = hemisphereCoverForSphere[i]](const SDK::Plane<double>& plane) {
            return Dot(plane.GetNormal(), n) >= 0;
        });

    }

    return 0;
}


