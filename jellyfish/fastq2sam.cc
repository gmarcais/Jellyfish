#include <fstream>
#include <string>
#include <limits>

#include <jellyfish/fastq2sam_cmdline.hpp>

int main(int argc, char* argv[]) {
  const std::string ext(".fastq");
  const std::string out_ext(".sam");

  fastq2sam_cmdline args(argc, argv);

  for(const auto& path : args.fastq_arg) {
    if(path.size() < ext.size() || path.substr(path.size() - ext.size()) != ext)
      fastq2sam_cmdline::error() << "Input must have '" << ext << "' extension";
    const std::string out_path = path.substr(0, path.size() - ext.size()) + out_ext;

    std::ifstream is(path);
    if(!is.good())
      fastq2sam_cmdline::error() << "Failed to open '" << path << '\'';
    std::ofstream os(out_path);
    if(!os.good())
      fastq2sam_cmdline::error() << "Failed to open '" << out_path << '\'';

    //    os << "@PG\tID:fastq2sam\tPN:fastq2sam\n";

    std::string name, seq, quals;
    int c = is.get();
    while(c == '@') {
      std::getline(is, name);
      std::getline(is, seq);
      if((c = is.get()) != '+') break;
      is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::getline(is, quals);
      os << name << "\t4\t*\t0\t0\t*\t*\t0\t0\t" << seq << '\t' << quals << '\n';
      c = is.get();
    }

    if(c != EOF)
      fastq2sam_cmdline::error() << "Input fastq file badly formatted. Unexpected characters '" << c << "' at position " << is.tellg();

    if(!os.good())
      fastq2sam_cmdline::error() << "Error while writing to file '" << out_path << '\'';
  }

  return 0;
}
