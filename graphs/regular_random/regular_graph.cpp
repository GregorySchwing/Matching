#include<iostream>
#include<algorithm>
#include<random>
#include<vector>
#include<set>
#include<map>
#include<stdexcept>

using Vec     = std::vector<int>;
using SetPair = std::set<std::pair<int,int>>;
using Map     = std::map<int,int>;
std::mt19937 rng;  // Mersenne Twister 19937 generator

void print_edges(const SetPair& ed){
	std::cout << "[";
	for (const auto& [a,b] : ed) {
		std::cout << "(" << a << "," << b << "), ";
	}
	std::cout << "]\n";
}

bool _is_suitable(const SetPair& edges, const Map& potential_edges){
	if(potential_edges.size() == 0) {
		return true;
	}
	for (const auto& [a,_]: potential_edges){
		for (const auto& [b,_]: potential_edges) {
			if (a == b) { break;}
			int s1,s2;
			if (a>b){
				s1 = b;
				s2 = a;
			}
			else{
				s1 = a;
				s2 = b;
			}
			if (edges.find(std::pair{s1,s2}) == edges.end()){
				return true;
			}
		}
	}
	return false;
}

SetPair _random_regular_graph(int n, int d) {
	if ((n*d) % 2 != 0  or d >= n) {
	       throw std::invalid_argument("n*d must be even and d must be less than n.");
	}	       
	SetPair edges {};

	Vec v {};
	for (int i=0; i<d; ++i){
		for (int j=0; j<n; ++j){
			v.push_back(j);
		}
	}
	while (v.size() > 0){
		Map potential_edges {};
		std::shuffle(v.begin(), v.end(), rng);  // Use the random number generator
			for (auto it = v.begin(); it != v.end(); ++it) {
				int s1,s2;
				s1 = *it;
				++it;
				s2 = *it;
				if(s1>s2){
					int tmp = s1;
					s1 = s2;
					s2 = tmp;
				}
				if ((s1 != s2) and (edges.find(std::pair{s1,s2}) == edges.end())){
					edges.insert(std::pair{s1,s2});
				}
				else {
					potential_edges[s1] += 1;
					potential_edges[s2] += 1;
				}
			}
		if (!_is_suitable(edges,potential_edges)){
			return SetPair{};
		}
		v.clear();
		for(auto& [a,b] : potential_edges){
			while(b!=0){
				v.push_back(a);
				--b;
			}
		}
		potential_edges.clear();
	}
	return edges;
}


SetPair random_regular_graph(int n, int d) {
	auto ed = _random_regular_graph(n,d);
	while(ed.size() == 0){
		ed = random_regular_graph(n,d);
	}
	return ed;	
}
#include <fstream>
#include <iostream>
void write_mtx(const SetPair& edges, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Writing header
    file << "%%MatrixMarket matrix coordinate pattern symmetric\n";
    file << edges.size() << " " << edges.size() << " " << edges.size() << "\n";

    // Writing edges
    for (const auto& [a, b] : edges) {
        file << a + 1 << " " << b + 1 << "\n"; // Adding 1 to indices since Matrix Market uses 1-based indexing
    }

    file.close();
}

int main(int argc, char const *argv[])
{
	rng.seed(123);
	int n = atoi(argv[1]); // Number of nodes (size of the graph)
	int d = atoi(argv[2]); // degree of each node in the graph
	std::string output_file = argv[3];
	SetPair ed = random_regular_graph(n,d);
    // Writing the graph to a Matrix Market file
    write_mtx(ed, output_file);
	return 0;
}
