#include "zincPrinter.hh"

using namespace llvm;

char pdg::MiniZincPrinter::ID = 0;

bool DEBUGZINC;

cl::opt<bool, true> DEBUG_ZINC("zinc-debug", cl::desc("print debug messages"), cl::value_desc("print debug messages"), cl::location(DEBUGZINC), cl::init(false));


void pdg::MiniZincPrinter::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<ProgramDependencyGraph>();
  AU.setPreservesAll();
}

bool pdg::MiniZincPrinter::runOnModule(Module &M)
{

  auto _PDG = &ProgramGraph::getInstance();
  std::map<std::string, std::vector<std::string> > outputEnums;
  std::map<std::string, std::vector<std::string> > outputArrays;


  std::vector<std::vector<Edge*>> PDG_edges(25,std::vector<Edge*>()) ;

  std::vector<std::vector<Node*>> PDG_nodes(25,std::vector<Node*>());
  std::map<std::string,int> nodeID2enum;
  std::map<std::string,int> edgeID2enum;
  



  _PDG->build(M);

  std::ofstream outFile;
  outFile.open ("pdg_data.dzn");




  for (auto node_iter = _PDG->begin(); node_iter != _PDG->end(); ++node_iter)
  {
    auto node = *node_iter;
    // we don't care about this type
    if (node->getNodeType() == pdg::GraphNodeType::FUNC)
    {
      continue;
    }

    for (auto out_edge : node->getOutEdgeSet())
    {
      // don't care about these yet
      if (out_edge->getEdgeType() == pdg::EdgeType::IND_CALL || out_edge->getEdgeType() == pdg::EdgeType::GLOBAL_DEP || out_edge->getEdgeType() == pdg::EdgeType::TYPE_OTHEREDGE)
      {
          continue;
      }
      Edge* edge = out_edge;
      PDG_edges[static_cast<int>(out_edge->getEdgeType())].push_back(edge);
    }
    PDG_nodes[static_cast<int>(node->getNodeType())].push_back(node);
  }

  int node_index = 1;
  for (auto& enums: PDG_nodes)
  {
    for (auto& nodes: enums)
    {
      Node* node = nodes; 
      std::string str;
    raw_string_ostream OS(str);
    std::string node_id = "";
    raw_string_ostream getNodeID(node_id);

    if (DEBUGZINC)
    {
        if (node->getValue() != nullptr)
        errs() << "node: " << node->getNodeID() << " annotation:" << node->getAnno() <<  " - " << pdgutils::rtrim(OS.str()) << " - " << pdgutils::getNodeTypeStr(node->getNodeType()) << "\n" ;
        else
        errs() << "node: " << node->getNodeID() << " annotation:" << node->getAnno() << " - " <<  pdgutils::getNodeTypeStr(node->getNodeType()) << "\n" ;

    }


    Value* val = node->getValue();

    FuncWrapperMap Fwm = _PDG->getFuncWrapperMap();
    Node* func_node = nullptr;
    if (node->getFunc())
    {
      func_node = Fwm[node->getFunc()]->getEntryNode();
    }
   
    
    if (func_node != nullptr)
    {
      getNodeID << func_node->getNodeID();
      // errs() << "Adding Function" << *val << "\n";
      outputArrays["hasFunction"].push_back(getNodeID.str()); 
    }
    else
    {
      // needs to be assigned to something or minizinc will complain
      outputArrays["hasFunction"].push_back("1"); 
    }

    if (val != nullptr)
    {
      if (Function* f = dyn_cast<Function>(val))
        OS << f->getName().str() << "\n";
      else
        OS << *val << "\n";
    }


    node_id = "";
    getNodeID << node->getNodeID();
    
    outputEnums[pdgutils::getNodeTypeStr(node->getNodeType())].push_back(getNodeID.str());
    nodeID2enum[getNodeID.str()] = node_index;
    // outputEnums["PDGNode"].push_back(getNodeID.str());

    // Need to make sure that these arrays sync up with the concatinated data
    outputArrays["hasCle"].push_back(node->getAnno()); 
    node_index++;
    }

  }

  //print edges
  // outFile << "=============== Edge Set ===============\n";

  int edge_index = 1;
  for (auto& enums: PDG_edges)
  {
    for (auto& edges: enums)
    {

      Edge* out_edge = edges;
      std::string edge_id;
      edge_id = "";
      raw_string_ostream getEdgeID(edge_id);
      if (DEBUGZINC)
      {
        errs() << "edge: " << out_edge->getEdgeID() << " / " << "src[" << out_edge->getSrcNode()->getNodeID() << "] / " << " dst[" << out_edge->getDstNode()->getNodeID() << "]" << " / " << pdgutils::getEdgeTypeStr(out_edge->getEdgeType()) << "\n";
        errs() << "Label Src: " << out_edge->getSrcNode()->getAnno() << " Label Dest: " << out_edge->getDstNode()->getAnno() << "\n";
      }
      getEdgeID << out_edge->getEdgeID();
      outputEnums[pdgutils::getEdgeTypeStr(out_edge->getEdgeType())].push_back(getEdgeID.str());
      edgeID2enum[getEdgeID.str()] = edge_index;
      
      // outputEnums["PDGEdge"].push_back(getEdgeID.str());
      edge_id = "";
      getEdgeID << out_edge->getSrcNode()->getNodeID();
      outputArrays["hasSource"].push_back(getEdgeID.str());
      edge_id = "";
      getEdgeID << out_edge->getDstNode()->getNodeID();
      outputArrays["hasDest"].push_back(getEdgeID.str());

      edge_index++;


      // switch (out_edge->getEdgeType())
      // {
      //   case EdgeType::CONTROLDEP_CALLINV:
      //   case EdgeType::CONTROLDEP_ENTRY:
      //   case EdgeType::CONTROLDEP_BR:
      //   case EdgeType::CONTROLDEP_CALLRET:
      //     getEdgeID << out_edge->getEdgeID();
      //     edge_id = "";
      //     outputEnums["ControlDep"].push_back(getEdgeID.str());
      //     edge_id = "";
      //     getEdgeID << out_edge->getSrcNode()->getNodeID();
      //     outputArrays["ControlDep_hasSource"].push_back(getEdgeID.str());
      //     edge_id = "";
      //     getEdgeID << out_edge->getDstNode()->getNodeID();
      //     outputArrays["ControlDep_hasDest"].push_back(getEdgeID.str());
      // }

      // switch (out_edge->getEdgeType())
      // {
      //   case EdgeType::CONTROLDEP_CALLINV:
      //     getEdgeID << out_edge->getEdgeID();
      //     edge_id = "";
      //     outputEnums["ControlDep_CallInv"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getSrcNode()->getNodeID();
      //     // outputArrays["ControlDep_CallInv_hasSource"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getDstNode()->getNodeID();
      //     // outputArrays["ControlDep_CallInv_hasDest"].push_back(getEdgeID.str());
      //     break;
      //   case EdgeType::CONTROLDEP_ENTRY:
      //     getEdgeID << out_edge->getEdgeID();
      //     edge_id = "";
      //     outputEnums["ControlDep_Entry"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getSrcNode()->getNodeID();
      //     // outputArrays["ControlDep_Entry_hasSource"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getDstNode()->getNodeID();
      //     // outputArrays["ControlDep_Entry_hasDest"].push_back(getEdgeID.str());
      //     break;
      //   case EdgeType::CONTROLDEP_BR:
      //     getEdgeID << out_edge->getEdgeID();
      //     edge_id = "";
      //     outputEnums["ControlDep_Br"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getSrcNode()->getNodeID();
      //     // outputArrays["ControlDep_Br_hasSource"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getDstNode()->getNodeID();
      //     // outputArrays["ControlDep_Br_hasDest"].push_back(getEdgeID.str());
      //     break;
      //   case EdgeType::CONTROLDEP_CALLRET:
      //     getEdgeID << out_edge->getEdgeID();
      //     edge_id = "";
      //     outputEnums["ControlDep_CallRet"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getSrcNode()->getNodeID();
      //     // outputArrays["ControlDep_CallRet_hasSource"].push_back(getEdgeID.str());
      //     // edge_id = "";
      //     // getEdgeID << out_edge->getDstNode()->getNodeID();
      //     // outputArrays["ControlDep_CallRet_hasDest"].push_back(getEdgeID.str());
      //     break;
      //   }

      if(out_edge->getEdgeType() == EdgeType::CONTROLDEP_CALLRET)
      {
          for (auto edge : out_edge->getDstNode()->getOutEdgeSet())
          {
            if (edge->getEdgeType() == EdgeType::CONTROLDEP_CALLINV)
            {
              edge_id = "";
              getEdgeID << edge->getEdgeID();
              outputArrays["invForRet"].push_back(getEdgeID.str());
              break;
            }
          }
        
      }
      
      
      
    }
  }

  for(auto &pair : outputEnums)
  {
    // std::reverse(pair.second.begin(), pair.second.end());
    outFile << pair.first << " = { ";
    bool first = true;
    for(auto const& i : pair.second) {
      if (first)
        outFile << "ID" + i ;
      else 
        outFile << ", " << "ID" + i ;
      first = false;
    }
    outFile << "};   \n ";
  }

  std::ofstream outFileCle;
  outFileCle.open ("init_cle.mzn");

  for(auto &pair : outputArrays)
  {
    // std::reverse(pair.second.begin(), pair.second.end());
    bool first = true;
    
    if(pair.first == "hasCle")
    {
      int index = 0;
      for(auto const& i : pair.second) {

        if(i != "None")
          outFileCle <<  "constraint hasCle[" << index << "]=" << i << "; \n";   
        index++;     
        // if (first)
        //   outFile <<  i;
        // else 
        //   outFile << ", " << i;
        first = false;
      }

      // for(auto const& i : pair.second) {
      //   if (first)
      //     // outFile <<  i;
      //     outFile <<  i;
      //   else 
      //     outFile << ", " << i;
      //   first = false;
      // }
    }
    else
    {
      outFile << pair.first << " = [ ";
      for(auto const& i : pair.second) {
        if (first)
          outFile << nodeID2enum[i];
        else 
          outFile << ", " << nodeID2enum[i];
        first = false;
      }
       outFile << "];   \n ";
    }
   
  }

  outFileCle.close();
  outFile.close();
}


static RegisterPass<pdg::MiniZincPrinter>
    ZINCPRINTER("minizinc",
               "Dump PDG data in minizinc format",
               false, false);