
#include <json.hpp>
#include <CXXGraph/CXXGraph.hpp>

#include <fstream>
#include <iostream>

#include <memory>

using std::make_shared;
using json = nlohmann::json;


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

	void from_json(const json& j, task& t) {
		j.at("name").get_to(t.name);
		j.at("machine").get_to(t.machine);
		j.at("processing_time").get_to(t.processing_time);
		j.at("predecessor").get_to(t.predecessor);
	}
} // namespace sched

int main()
{
	std::ifstream f("job_cell.json");
	json data = json::parse(f);
	f.close();
	//int n_machines = data["machines"].size();
    //int n_jobs = data["jobs"].size();
	//std::cout << "there are " << n_machines << " machines" << std::endl;
    //std::cout << "there are " << n_jobs << " jobs" << std::endl;

	std::vector<std::string> machinelist;
	auto j_machines = data["machines"];
    for (auto m : data["machines"].items())
    {
		machinelist.push_back(m.key());
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
		//std::cout << e.getNodePair().first.get()->getUserId()
		//	<< " --- " << e.getWeight()
		//	<< " --> " << e.getNodePair().second.get()->getUserId() << std::endl;
		//std::cout << e << std::endl;
	}
	CXXGraph::Graph<int> graph2(edgeSet2);
	auto bf2 = graph2.bellmanford(nodeU, nodeV);
	graph2.writeToDotFile("C:\\repos\\scheduling", "output", "output_graph");

	std::cout << "Done";
    return 0;
}