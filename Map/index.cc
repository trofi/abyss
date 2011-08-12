#include "config.h"
#include "FastaIndex.h"
#include "FMIndex.h"
#include "IOUtil.h"
#include <algorithm>
#include <cctype> // for toupper
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

using namespace std;

#define PROGRAM "abyss-index"

static const char VERSION_MESSAGE[] =
PROGRAM " (" PACKAGE_NAME ") " VERSION "\n"
"Written by Shaun Jackman.\n"
"\n"
"Copyright 2011 Canada's Michael Smith Genome Science Centre\n";

static const char USAGE_MESSAGE[] =
"Usage: " PROGRAM " [OPTION]... FILE\n"
"Build an FM-index of FILE and store it in FILE.fm.\n"
"\n"
"  -s, --sample=N          sample the suffix array [4]\n"
"  -d, --decompress        decompress the index FILE\n"
"  -c, --stdout            write output to standard output\n"
"  -v, --verbose           display verbose output\n"
"      --help              display this help and exit\n"
"      --version           output version information and exit\n"
"\n"
"Report bugs to <" PACKAGE_BUGREPORT ">.\n";

namespace opt {
	/** Sample the suffix array. */
	static unsigned sampleSA = 4;

	/** Decompress the index. */
	static bool decompress;

	/** Write output to standard output. */
	static bool toStdout;

	/** Verbose output. */
	static int verbose;
}

static const char shortopts[] = "cds:v";

enum { OPT_HELP = 1, OPT_VERSION };

static const struct option longopts[] = {
	{ "decompress", no_argument, NULL, 'd' },
	{ "sample", required_argument, NULL, 's' },
	{ "stdout", no_argument, NULL, 'c' },
	{ "help", no_argument, NULL, OPT_HELP },
	{ "version", no_argument, NULL, OPT_VERSION },
	{ NULL, 0, NULL, 0 }
};

/** Index the specified FASTA file. */
static void indexFasta(const string& faPath, const string& faiPath)
{
	cerr << "Reading `" << faPath << "'...\n";
	FastaIndex fai;
	fai.index(faPath);

	cerr << "Writing `" << faiPath << "'...\n";
	ofstream out(faiPath.c_str());
	assert_good(out, faiPath);
	out << fai;
	out.flush();
	assert_good(out, faiPath);
}

/** Build an FM index of the specified file. */
static void buildFMIndex(FMIndex& fm, const char* path)
{
	if (opt::verbose > 0)
		std::cerr << "Reading `" << path << "'...\n";
	std::vector<FMIndex::value_type> s;
	readFile(path, s);
	transform(s.begin(), s.end(), s.begin(), ::toupper);
	fm.setAlphabet("-ACGT");
	fm.assign(s.begin(), s.end());
	fm.sampleSA(opt::sampleSA);
}

int main(int argc, char **argv)
{
	bool die = false;
	for (int c; (c = getopt_long(argc, argv,
					shortopts, longopts, NULL)) != -1;) {
		istringstream arg(optarg != NULL ? optarg : "");
		switch (c) {
			case '?': die = true; break;
			case 'c': opt::toStdout = true; break;
			case 'd': opt::decompress = true; break;
			case 's': arg >> opt::sampleSA; assert(arg.eof()); break;
			case 'v': opt::verbose++; break;
			case OPT_HELP:
				cout << USAGE_MESSAGE;
				exit(EXIT_SUCCESS);
			case OPT_VERSION:
				cout << VERSION_MESSAGE;
				exit(EXIT_SUCCESS);
		}
	}

	if (argc - optind < 1) {
		cerr << PROGRAM ": missing arguments\n";
		die = true;
	}

	if (argc - optind > 1) {
		cerr << PROGRAM ": too many arguments\n";
		die = true;
	}

	if (die) {
		cerr << "Try `" << PROGRAM
			<< " --help' for more information.\n";
		exit(EXIT_FAILURE);
	}

	if (opt::decompress) {
		// Decompress the index.
		string fmPath(argv[optind]);
		if (fmPath.size() < 4
				|| !equal(fmPath.end() - 3, fmPath.end(), ".fm"))
			fmPath.append(".fm");
		string faPath(fmPath, 0, fmPath.size() - 3);

		ifstream in(fmPath.c_str());
		assert_good(in, fmPath);
		FMIndex fmIndex;
		in >> fmIndex;
		assert_good(in, fmPath);
		in.close();

		ofstream fout;
		if (!opt::toStdout)
			fout.open(faPath.c_str());
		ostream& out = opt::toStdout ? cout : fout;
		assert_good(out, faPath);
		fmIndex.decompress(
				ostream_iterator<FMIndex::value_type>(out, ""));
		out.flush();
		assert_good(out, faPath);

		ostringstream ss;
		ss << faPath << ".fai";
		string faiPath(ss.str());
		in.open(faiPath.c_str());
		FastaIndex faIndex;
		if (in) {
			in >> faIndex;
			faIndex.writeFASTAHeaders(out);
		}
		return 0;
	}

	const char* faPath(argv[optind]);
	ostringstream ss;
	ss << faPath << ".fm";
	string fmPath = opt::toStdout ? "-" : ss.str();
	ss.str("");
	ss << faPath << ".fai";
	string faiPath(ss.str());

	if (!opt::toStdout)
		indexFasta(faPath, faiPath);

	FMIndex fm;
	buildFMIndex(fm, faPath);

	cerr << "Writing `" << fmPath << "'...\n";
	ofstream fout;
	if (!opt::toStdout)
		fout.open(fmPath.c_str());
	ostream& out = opt::toStdout ? cout : fout;
	assert_good(out, fmPath);
	out << fm;
	out.flush();
	assert_good(out, fmPath);

	return 0;
}