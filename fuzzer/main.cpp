#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <libfuzzer/Fuzzer.h>
#include <libfuzzer/ContractABI.h>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <unistd.h>

using namespace std;
using namespace fuzzer;
namespace pt = boost::property_tree;

int main(int argc, char* argv[]) {
  if (argc > 1) {
    /* Accept the first one as file name */
    string filename(argv[1]);
    string contractName(argv[2]);
    ifstream file(filename);
    if (!file.is_open()) {
      cout << "[x] File " << filename << " is not found " << endl;
      return 0;
    }
    /* Use ptree to read */
    pt::ptree root;
    pt::read_json(filename, root);
    pt::ptree::path_type abiPath("contracts/"+ contractName +"/abi", '/');
    pt::ptree::path_type binPath("contracts/"+ contractName +"/bin", '/');
    auto abiJson = root.get<string>(abiPath);
    auto bin = root.get<string>(binPath);
    ContractABI ca(abiJson);
    Fuzzer fuzzer(fromHex(bin), ca);
    fuzzer.start();
    return 0;
  }
  cout << "[x] Provide json file" << endl;
  return 0;
}
