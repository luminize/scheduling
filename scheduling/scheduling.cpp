
#include <json.hpp>
#include <CXXGraph/CXXGraph.hpp>

#include <fstream>
#include <iostream>

#include <memory>

using std::make_shared;
using json = nlohmann::json;

// Driver function to sort the vector elements
// by second element of pairs
bool sortbysec(const std::pair<std::string, double>& a,
	const std::pair<std::string, double>& b)
{
	return (a.second < b.second);
}





/* json layout
"name": "task 1.1",
"machine" : "m1",
"processing_time" : 10.0,
"predecessor" : ""
*/
namespace sched {
	
	struct task {
		std::string id;
		std::string name;
		std::string machine;
		double processing_time;
		double delay_time;
		double completion_time;
		std::string predecessor;
		std::string parent_job;
		bool is_a_predecessor = false;
		int graph_id = 0;
	};

	struct job {
		std::string name;
		std::vector<sched::task> t;
	};

	struct machine {
		std::string id;
		std::string name;
		std::vector<std::shared_ptr<sched::task>> tasks_on_machine;
	};

	void from_json(const json& j, task& t) {
		j.at("name").get_to(t.name);
		j.at("machine").get_to(t.machine);
		j.at("processing_time").get_to(t.processing_time);
		j.at("predecessor").get_to(t.predecessor);
	}
} // namespace sched

std::vector<CXXGraph::shared<const CXXGraph::Edge<int>>> find_edges(CXXGraph::shared<const CXXGraph::Node<int>> target_node, CXXGraph::T_EdgeSet<int> &edgeSet, bool incoming) {
	std::vector<CXXGraph::shared<const CXXGraph::Edge<int>>> edges{};
	CXXGraph::shared<const CXXGraph::Node<int>> temp_node;
	for (auto& e : edgeSet) {
		auto edge = e.get();
		if (incoming) {
			temp_node = edge->getNodePair().second;
		}
		else {
			temp_node = edge->getNodePair().first;
		}
		if (temp_node == target_node) {
			edges.push_back(e);
		}
	}
	return edges;
}

std::vector<CXXGraph::shared<const CXXGraph::Node<int>>> find_node_connections(CXXGraph::Graph<int>& graph, int nr_of_edges, bool incoming) {
	std::vector<CXXGraph::shared<const CXXGraph::Node<int>>> node_list{};
	auto edges = graph.getEdgeSet();
	auto nodes = graph.getNodeSet();
	// for each node, look if there is exactly one node as second in the nodepair
	// if that is the case at the end of the loop thru the edges, add that node to
	// the vector.
	for (auto n : nodes) {
		CXXGraph::shared<const CXXGraph::Node<int>> temp_node = n;
		auto edges_vector = find_edges(temp_node, edges, incoming);
		if (edges_vector.size() == nr_of_edges) {
			node_list.push_back(n);
		}
	}
	return node_list;
}

int main()
{
	std::ifstream f("job_cell.json");
	json data = json::parse(f);
	f.close();
	//int n_machines = data["machines"].size();
    //int n_jobs = data["jobs"].size();
	//std::cout << "there are " << n_machines << " machines" << std::endl;
    //std::cout << "there are " << n_jobs << " jobs" << std::endl;

	std::vector<sched::machine> machinelist;
	auto j_machines = data["machines"];
    for (auto m : data["machines"].items())
    {
		sched::machine machine;
		machine.id = m.key();
		machine.name = m.value();
		machinelist.push_back(machine);
    }

	auto j_jobs = data["jobs"];
	std::vector<sched::task> tasklist;
	std::vector<sched::job> joblist;
	std::vector<CXXGraph::DirectedWeightedEdge<int>> edgelist;
	

	// fill variables with json data
	for(auto& j : data["jobs"].items() )
	{
		json j_content = j.value();
		sched::job temp_job;
		temp_job.name = j.key();
		for (auto& t : j_content.items()) {
			json t_content = t.value();
			//std::cout << t.key() << " holds " << t_content << std::endl << std::endl;
			auto temp_t = t_content.template get<sched::task>();
			temp_t.id = temp_job.name + t.key();
			// if no 'j' in precedessor value, then update predecessor value
			if (!((temp_t.predecessor[0] == 'j') || (temp_t.predecessor[0] == 'J') || temp_t.predecessor == ""))
			{
				temp_t.predecessor = temp_job.name + temp_t.predecessor;
			}
			temp_job.t.push_back(temp_t); 
		}
		joblist.push_back(temp_job);
	}

	// we've now taken over all of the jobs and tasks.
	// iterate over the job list, and fill the task list
	//   each task needs adding the parent job name
	//   each task name needs modifying it's name so we do not get nodes with identical names.

	for (auto& jb : joblist) {
		for (auto& tsk : jb.t) {
			//tsk.name = jb.name + tsk.name;
			tsk.parent_job = jb.name;
			tasklist.push_back(tsk);
		}
	}
	int graph_id = 2;
	//std::vector<CXXGraph::Node<sched::task>> nodelist;
	std::vector<CXXGraph::Node<int>> nodelist;

	CXXGraph::Node<int> nodeU("U", 0);
	CXXGraph::Node<int> nodeV("V", 1);
	nodelist.push_back(nodeU);
	nodelist.push_back(nodeV);
	std::vector<std::string> node_to_task_map{"U", "V"};

	for (auto& tsk : tasklist) {
		tsk.graph_id = graph_id;
		CXXGraph::Node<int> temp_node(tsk.id, tsk.graph_id);
		nodelist.push_back(temp_node);
		node_to_task_map.push_back(tsk.id);
		graph_id++;
	}
	
	// iterate thru the nodelist and find nodes that do not have a predecessor
	// those nodes are the start of jobs. let those point to node U.
	
	CXXGraph::id_t edge_iterator = 1;

	for (auto& t : tasklist) {

		// first, check on which machine this task should run
		bool machine_found = false;
		int machine_iterator = 0;
		while ((machine_iterator < machinelist.size()) && (!machine_found) ) {
			if (machinelist[machine_iterator].id == t.machine) {
				machinelist[machine_iterator].tasks_on_machine.push_back(make_shared<sched::task>(t));
				machine_found = true;
			}
			machine_iterator++;
		}

		// if no predecessor, then edge from taskU node
		// get task id to search for in the nodelist
		std::string current_task_id = t.id;
		int current_graph_id = t.graph_id;
		double weight = 0.0;
		int iterator = 0;
		CXXGraph::Node<int> predecessor_node("", 0);
		CXXGraph::Node<int> current_node = nodelist[current_graph_id];
		// no predecessor for this task
		if (t.predecessor == "") {
			// use node "U" as precedessor
			CXXGraph::DirectedWeightedEdge<int> temp_edge(edge_iterator, nodeU, current_node, weight);
			edgelist.push_back(temp_edge);
			edge_iterator++;
		}
		// this task has a predecessor
		else
		{
			// todo: check for existence of named predecessor
			std::string predecessor_name = t.predecessor;
			// we do not start at nodes U and V.
			int node_to_task_map_iterator = 2;
			bool predecessor_found = false;
			while (node_to_task_map_iterator < node_to_task_map.size() && !predecessor_found) {
				// iterate the map, and the index found will be the index
				// that holds the node in the nodelist.
				if (node_to_task_map[node_to_task_map_iterator] == t.predecessor) {
					predecessor_node = nodelist[node_to_task_map_iterator];
					weight = tasklist[node_to_task_map_iterator - 2].processing_time;
					tasklist[node_to_task_map_iterator - 2].is_a_predecessor = true;
					predecessor_found = true;
				}
				node_to_task_map_iterator++;
			}
			CXXGraph::DirectedWeightedEdge<int> temp_edge(edge_iterator, predecessor_node, current_node, weight);
			edgelist.push_back(temp_edge);
			edge_iterator++;
		}
	}

	// now check if there are no nodes which are predecessor
	for (auto& t : tasklist) {
		if (!t.is_a_predecessor){
			CXXGraph::Node<int> predecessor_node("", 0);
			CXXGraph::DirectedWeightedEdge<int> temp_edge(edge_iterator, nodelist[t.graph_id], nodeV, t.processing_time);
			edgelist.push_back(temp_edge);
			edge_iterator++;
		}
	}

	CXXGraph::T_EdgeSet<int> edgeSet;
	for (auto e : edgelist) {
		edgeSet.insert(make_shared<CXXGraph::DirectedWeightedEdge<int>>(e));
		std::cout << e.getNodePair().first.get()->getUserId()
			<< " --- " << e.getWeight()
			<< " --> " << e.getNodePair().second.get()->getUserId() << std::endl;
	}
	CXXGraph::Graph<int> graph(edgeSet);

	auto toposort = graph.topologicalSort();
	for (auto s : toposort.nodesInTopoOrder) {
		// print in tolological order
		std::cout << s.getUserId() << std::endl;
	}

	auto dfs = graph.depth_first_search(nodeU);
	auto bfs = graph.breadth_first_search(nodeU);
	auto cycle = graph.isCyclicDirectedGraphBFS();
	
	// returns 14 for the shortes path
	auto bf = graph.bellmanford(nodeU, nodeV);
	
	CXXGraph::T_EdgeSet<int> edgeSet2;
	for (auto e : edgelist) {
		e.setWeight(e.getWeight() * -1.0);
		edgeSet2.insert(make_shared<CXXGraph::DirectedWeightedEdge<int>>(e));
	}
	CXXGraph::Graph<int> graph2(edgeSet2);
	auto bf2 = graph2.bellmanford(nodeU, nodeV);
	double crit_path_length = bf2.result;
	graph2.writeToDotFile("C:\\repos\\scheduling", "output", "output_graph");

	auto dist = graph.getAdjMatrix();
	auto eul = graph.getLaplacianMatrix();
	auto trans = graph.getTransitionMatrix();
	auto deg = graph.getDegreeMatrix();

	auto mst = graph.kruskal();
	auto bor = graph2.boruvka();
	
	auto ns1 = graph2.getNodeSet();
	std::cout << "ns1 : " << ns1.size() << std::endl;
	std::cout << "es2 : " << edgeSet2.size() << std::endl;

	std::vector<std::string> endnodes_queue;
	// first get all the nodes pointing to the end node
	endnodes_queue.push_back(nodeV.getUserId());
	for (auto es:graph2.getEdgeSet()) {
		if (es.get()->getNodePair().second.get()->getData() == nodeV.getData()) {
			auto w_1 = nodelist[es.get()->getNodePair().first.get()->getData()];
			endnodes_queue.push_back(w_1.getUserId());
			std::cout << w_1 << std::endl;
		}
	}

	endnodes_queue.erase(endnodes_queue.begin());

	auto ns2 = graph2.getNodeSet();
	std::cout << "ns2 : " << ns2.size() << std::endl;
	std::cout << "es2 : " << edgeSet2.size() << std::endl;

	// find the node graph_id from tasklist by looking up UserId
	// use that to get the node from the nodelist
	// have a shortest path to that node
	// use its weight to add to the result to check if it
	// is on the critical path

	std::vector<std::string> copy_endnodes_queue = endnodes_queue;
	for (auto curr_node : ns2) {
		auto curr_node_user_id = curr_node.get()->getUserId();
		for (int i = 0; i < copy_endnodes_queue.size(); i++) {
			if (curr_node_user_id == copy_endnodes_queue[i]) {
				CXXGraph::Node<int> temp_node("", 0);
				double length_to_node = 0;
				double temp_processing_time = 0;
				for (auto t : tasklist) {
					if (t.id == curr_node_user_id) {
						temp_node = nodelist[t.graph_id];
						temp_processing_time = -1.0 * t.processing_time;
					}
				}
				auto bfres = graph2.bellmanford(nodeU, temp_node);
				std::cout << std::endl << copy_endnodes_queue[i] << bfres.result << std::endl;
				if ((bfres.result + temp_processing_time) == crit_path_length) {
					std::cout << bfres.result << " + " << temp_processing_time << " == " << crit_path_length << std::endl;
					std::cout << copy_endnodes_queue[i] << " can be on critical path!" << std::endl;
				}
				copy_endnodes_queue.erase(copy_endnodes_queue.begin() + i);
			}
		}
	}

	// if we have the result of the critical path, we can find the path by copying
	// everything with a value < 0.
	// Working backwards from V towards U.
	// when a node has 1 predecessor, consume that node
	// Everywhere that there is a node which has more than 1 precesessor nodes, check that
	// the value of predecessor + weight equal the current value of the node.
	// if that is true for more than one, copy the current backwards path, and consume the
	// nodes from that path.

	/*
	result	-22.500000000000000	double

	[0]	("U", 0.0000000000000000)		std::pair<std::string,double>
	[1]	("j2t2", -8.0000000000000000)	std::pair<std::string,double>
	[2]	("j2t1", 0.0000000000000000)	std::pair<std::string,double>
	[3]	("j1t1", 0.0000000000000000)	std::pair<std::string,double>
	[4]	("j2t3", -11.000000000000000)	std::pair<std::string,double>
	[5]	("j1t2", -10.000000000000000)	std::pair<std::string,double>
	[6]	("U", 0.0000000000000000)		std::pair<std::string,double>
	[7]	("U", 0.0000000000000000)		std::pair<std::string,double>
	[8]	("j3t1", 0.0000000000000000)	std::pair<std::string,double>
	[9]	("j3t2", -4.0000000000000000)	std::pair<std::string,double>
	[10]("j1t3", -18.000000000000000)	std::pair<std::string,double>
	[11]("j2t4", -16.000000000000000)	std::pair<std::string,double>
	[12]("j3t3", -11.000000000000000)	std::pair<std::string,double>
	*/

	std::vector<std::stack<std::string>> paths;
	std::vector<std::pair<std::string, double>> results{};

	results = bf2.node_and_value;
	
	// https://www.geeksforgeeks.org/sort-vector-of-pairs-in-ascending-order-in-c/
	std::sort(results.begin(), results.end(), sortbysec);

	auto r1 = find_node_connections(graph2, 0, true);
	auto r2 = find_node_connections(graph2, 0, false);
	auto r3 = find_node_connections(graph2, 1, true);
	auto r4 = find_node_connections(graph2, 1, false);
	auto r5 = find_node_connections(graph2, 2, true);
	auto r6 = find_node_connections(graph2, 2, false);
	auto r7 = find_node_connections(graph2, 3, true);
	auto r8 = find_node_connections(graph2, 3, false);

	std::cout << "Done";
    return 0;
}
