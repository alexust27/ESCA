#include <fstream>

#include "DefectStorage.h"

using namespace std;

DefectStorage& DefectStorage::Instance()
{
	static DefectStorage instance;
	return instance;
}

void DefectStorage::SaveDefects()
{
	if (outputPath.empty())
	{
		return;
	}

	ofstream outf(outputPath);
	if (!outf.good())
	{
		return;
	}

    for (const auto& defect : defects) {
        outf << defect << endl;
    }

	outf.close();
}
