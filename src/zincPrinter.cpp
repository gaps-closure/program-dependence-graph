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
  std::map<std::string, std::vector<std::string> > outputEnumsPDGNode;
  std::map<std::string, Node* > nodeID2Node;
  std::map<std::string, std::vector<std::string> > outputEnumsPDGEdge;
  std::map<std::string, Edge* > edgeID2Edge;
  std::map<std::string, std::vector<std::string> > outputArrays;

 // Need to save store nodes and edges so they are sorted correctly for minizinc 
  std::vector<std::vector<Edge*>> PDG_edges(19,std::vector<Edge*>()) ;
  std::vector<std::vector<Node*>> PDG_nodes(18,std::vector<Node*>());

  std::map<std::string,int> nodeID2enum;
  std::map<std::string,int> edgeID2enum;

  std::map<std::string,int> nodeID2index;
  
  std::vector<std::string> nodeOrder{
    "Inst_FunCall",
    "Inst_Ret",
    "Inst_Br",
    "Inst_Other",
    "Inst",
    "VarNode_StaticGlobal",
    "VarNode_StaticModule",
    "VarNode_StaticFunction",
    "VarNode_StaticOther",
    "VarNode",
    "FunctionEntry",
    "Param_FormalIn",
    "Param_FormalOut",
    "Param_ActualIn",
    "Param_ActualOut",
    "Param",
    "Annotation_Var",
    "Annotation_Global",
    "Annotation_Other",
    "Annotation"
  };

  std::vector<std::string> edgeOrder{
    "ControlDep_CallInv",
    "ControlDep_CallRet",
    "ControlDep_Entry",
    "ControlDep_Br",
    "ControlDep_Other",
    "ControlDep",
    "DataDepEdge_DefUse",
    "DataDepEdge_RAW",
    "DataDepEdge_Ret",
    "DataDepEdge_Alias",
    "DataDepEdge",
    "Parameter_In",
    "Parameter_Out",
    "Parameter_Field",
    "Parameter",
    "Anno_Global",
    "Anno_Var",
    "Anno_Other",
    "Anno"
  };

  std::vector<std::string> arrayOrder{
    "hasSource",
    "hasDest",
    "hasFunction"
    // "invForRet"
  };


  // initialize output
  outputEnumsPDGNode["Inst_FunCall"] = std::vector<std::string>();
  outputEnumsPDGNode["Inst_Ret"] = std::vector<std::string>();
  outputEnumsPDGNode["Inst_Br"] = std::vector<std::string>();
  outputEnumsPDGNode["Inst_Other"] = std::vector<std::string>();
  outputEnumsPDGNode["VarNode_StaticGlobal"] = std::vector<std::string>();
  outputEnumsPDGNode["VarNode_StaticModule"] = std::vector<std::string>();
  outputEnumsPDGNode["VarNode_StaticFunction"] = std::vector<std::string>();
  outputEnumsPDGNode["VarNode_StaticOther"] = std::vector<std::string>();
  outputEnumsPDGNode["FunctionEntry"] = std::vector<std::string>();
  outputEnumsPDGNode["Param_FormalIn"] = std::vector<std::string>();
  outputEnumsPDGNode["Param_FormalOut"] = std::vector<std::string>();
  outputEnumsPDGNode["Param_ActualIn"] = std::vector<std::string>();
  outputEnumsPDGNode["Param_ActualOut"] = std::vector<std::string>();
  outputEnumsPDGNode["Annotation_Var"] = std::vector<std::string>();
  outputEnumsPDGNode["Annotation_Global"] = std::vector<std::string>();
  outputEnumsPDGNode["Annotation_Other"] = std::vector<std::string>();

  outputEnumsPDGEdge["ControlDep_CallInv"] = std::vector<std::string>();
  outputEnumsPDGEdge["ControlDep_Entry"] = std::vector<std::string>();
  outputEnumsPDGEdge["ControlDep_Br"] = std::vector<std::string>();
  outputEnumsPDGEdge["ControlDep_CallRet"] = std::vector<std::string>();
  outputEnumsPDGEdge["DataDepEdge_DefUse"] = std::vector<std::string>();
  outputEnumsPDGEdge["DataDepEdge_RAW"] = std::vector<std::string>();
  outputEnumsPDGEdge["DataDepEdge_Alias"] = std::vector<std::string>();
  outputEnumsPDGEdge["DataDepEdge_Ret"] = std::vector<std::string>();
  outputEnumsPDGEdge["Parameter_In"] = std::vector<std::string>();
  outputEnumsPDGEdge["Parameter_Out"] = std::vector<std::string>();
  outputEnumsPDGEdge["Parameter_Field"] = std::vector<std::string>();
  outputEnumsPDGEdge["Anno_Global"] = std::vector<std::string>();
  outputEnumsPDGEdge["Anno_Var"] = std::vector<std::string>();
  outputEnumsPDGEdge["Anno_Other"] = std::vector<std::string>();
  outputEnumsPDGEdge["ControlDep_Other"] = std::vector<std::string>();



  // _PDG->build(M);

  std::ofstream outFile;
  std::ofstream dbgFile;
  std::ofstream node2line;

  outFile.open ("pdg_instance.mzn");
  dbgFile.open ("pdg_data.csv");
  node2line.open ("node2lineNumber.txt");

  for (auto node_iter = _PDG->begin(); node_iter != _PDG->end(); ++node_iter)
  {
    auto node = *node_iter;
    // we don't care about this type
    if (node->getNodeType() == pdg::GraphNodeType::FUNC)
    {
      continue;
    }

    if (node->getNodeType() == pdg::GraphNodeType::INST_FUNCALL)
    {
      llvm::Instruction* inst = dyn_cast<llvm::Instruction>(node->getValue());
      if (isa<DbgInfoIntrinsic>(inst))
      {
        continue;
      }
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

        if( node->getNodeType() == pdg::GraphNodeType::FUNC_ENTRY)
          errs() << "Value: " << *(node->getFunc()) << "Line Number: " << node->getLineNumber() << "\n";


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
    
    outputEnumsPDGNode[pdgutils::getNodeTypeStr(node->getNodeType())].push_back(getNodeID.str());
    nodeID2Node[getNodeID.str()] = node;
    nodeID2enum[getNodeID.str()] = node_index;
    // outputEnums["PDGNode"].push_back(getNodeID.str());

    // Need to make sure that these arrays sync up with the concatinated data
    // outputArrays["hasCle"].push_back(node->getAnno()); 
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
      std::string src_id;
      std::string dest_id;
      edge_id = "";
      src_id = "";
      dest_id = "";
      raw_string_ostream getEdgeID(edge_id);
      raw_string_ostream getSrcID(src_id);
      raw_string_ostream getDstID(dest_id);
      getSrcID << out_edge->getSrcNode()->getNodeID();
      getDstID << out_edge->getDstNode()->getNodeID();
      if (out_edge->getSrcNode()->getNodeType() == pdg::GraphNodeType::INST_FUNCALL)
      {
        llvm::Instruction* inst = dyn_cast<llvm::Instruction>(out_edge->getSrcNode()->getValue());
        if (isa<DbgInfoIntrinsic>(inst))
        {
          continue;
        }
      }

       if (out_edge->getDstNode()->getNodeType() == pdg::GraphNodeType::INST_FUNCALL)
      {
        llvm::Instruction* inst = dyn_cast<llvm::Instruction>(out_edge->getDstNode()->getValue());
        if (isa<DbgInfoIntrinsic>(inst))
        {
          continue;
        }
      }



      if (DEBUGZINC)
      {
        errs() << "edge: " << out_edge->getEdgeID() << " / " << "src[" << out_edge->getSrcNode()->getNodeID() << "]" << "(" << nodeID2enum[getSrcID.str()] << ")" <<  "/ " << " dst[" << out_edge->getDstNode()->getNodeID() << "]" << "(" << nodeID2enum[getDstID.str()] << ")" << " / " << pdgutils::getEdgeTypeStr(out_edge->getEdgeType()) << "\n";
        errs() << "Label Src: " << out_edge->getSrcNode()->getAnno() << " Label Dest: " << out_edge->getDstNode()->getAnno() << "\n";
      }
      getEdgeID << out_edge->getEdgeID();
      outputEnumsPDGEdge[pdgutils::getEdgeTypeStr(out_edge->getEdgeType())].push_back(getEdgeID.str());
      edgeID2enum[getEdgeID.str()] = edge_index;
      edgeID2Edge[getEdgeID.str()] = out_edge;
      

      edge_index++;


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

  int index = 1;

  // outFile << "PDGNodes" << " = [ ";
 
  bool first = true;
  for(auto &id : nodeOrder)
  {
    for(auto const& i : outputEnumsPDGNode[id]) {
      // if (first)
      //   outFile <<  i ;
      // else 
      //   outFile << ", " <<  i ;
      first = false;
      nodeID2index[i] = index;
      index++;
    } 
  }
  // outFile << "];   \n ";

  index = 1;
  int class_idx = 0;
  int super_class_start = 1;
  int super_class_end = -1;
  int max_node_idx = 1;
  for(auto &id : nodeOrder)
  {
    if (class_idx == 4 || class_idx == 9 || class_idx == 15 || class_idx == 19)
    {
      outFile << "int: " << id << "_start" << " = " << super_class_start << ";\n";
      outFile << "int: " << id << "_end" << " = " << super_class_end << ";\n";
      super_class_start = super_class_end + 1;
      super_class_end = -1;
      class_idx++;
      continue;
    }
    // Need to subtract 1 because minizinc is inclusive of right index
    int endIdx = index + outputEnumsPDGNode[id].size() -1;
    if (endIdx < index)
    {
      outFile << "int: "  << id << "_start" << " = 0;\n";
      outFile << "int: "  << id << "_end" << " = -1;\n";
    }
    else
    {
      outFile << "int: "  << id << "_start" << " = " << index << ";\n";
      outFile << "int: "  << id << "_end" << " = " << endIdx << ";\n";
      // Need to progress to next set
      super_class_end = endIdx;
      max_node_idx = endIdx;
      index = endIdx + 1;
      
    }
    class_idx++;
  }

  outFile << "int: PDGNode_start = 1;\n";
  outFile << "int: PDGNode_end" << " = " << max_node_idx << ";\n";

  index = 1;
  for(auto &id : nodeOrder)
  {
    for(auto const& i : outputEnumsPDGNode[id]) {
      std::string valueStr;
      std::string nameStr;
      if (nodeID2Node[i]->getValue() == nullptr)
      {
        valueStr = "No Value";
        nameStr  = "None";
      }
      else
      {
        llvm::raw_string_ostream(valueStr) << *(nodeID2Node[i]->getValue());
        // escape quotes in the string
        std::string::size_type sz = 0;
        while ( ( sz = valueStr.find("\"", sz) ) != std::string::npos )
        {
            valueStr.replace(sz, 1, "\"\"");
            sz += 2;
        }

        nameStr  = nodeID2Node[i]->getValue()->getName();
      }
      
      dbgFile << "Node, " << index << ", " <<  id << ", " << i << ", \"" << valueStr << "\", " << outputArrays["hasFunction"][index-1] << ", na, na, " << nodeID2Node[i]->getFileName()  << ", " << nodeID2Node[i]->getLineNumber() << "\n";
      node2line << index << ", " << nameStr << ", " << nodeID2Node[i]->getFileName()  << ", " << nodeID2Node[i]->getLineNumber() << "\n";
      index++;
    } 
  }
  
  index = 1;
  class_idx = 0;
  super_class_start = 1;
  super_class_end = -1;
  int max_edge_idx = 1;
  for(auto &id : edgeOrder)
  {
    if (class_idx == 5 || class_idx == 10 || class_idx == 14 || class_idx == 18)
    {
      outFile << "int: "  << id << "_start" << " = " << super_class_start << ";\n";
      outFile << "int: "  << id << "_end" << " = " << super_class_end << ";\n";
      super_class_start = super_class_end + 1;
      super_class_end = -1;
      class_idx++;
      continue;
    }
    int endIdx = index + outputEnumsPDGEdge[id].size() -1;
    
    if (endIdx < index)
    {
      outFile << "int: "  << id << "_start" << " = 0;\n";
      outFile << "int: "  << id << "_end" << " = -1;\n";
    }
    else
    {
      outFile << "int: "  << id << "_start" << " = " << index << ";\n";
      outFile << "int: "  << id << "_end" << " = " << endIdx << ";\n";
      max_edge_idx = endIdx;
      super_class_end = endIdx;
      index = endIdx + 1;
    }
    class_idx++;
  }
  outFile << "int: PDGEdge_start = 1;\n";
  outFile << "int: PDGEdge_end" << " = " << max_edge_idx << ";\n";

  index = 1;
  for(auto &id : edgeOrder)
  {
     for(auto const &i : outputEnumsPDGEdge[id] )
     {
        Edge* edg = edgeID2Edge[i];
        std::string src_id;
        std::string dest_id;
        src_id = "";
        dest_id = "";
        raw_string_ostream getSrcID(src_id);
        raw_string_ostream getDstID(dest_id);
        getSrcID << edg->getSrcNode()->getNodeID();
        getDstID << edg->getDstNode()->getNodeID();
        
        outputArrays["hasSource"].push_back(std::to_string(nodeID2index[getSrcID.str()]));
        outputArrays["hasDest"].push_back(std::to_string(nodeID2index[getDstID.str()]));
     }
  }

  index = 1;
  for(auto &id : edgeOrder)
  {
    for(auto const& i : outputEnumsPDGEdge[id]) {
      dbgFile << "Edge, " << index << ", " << id << ", " << i << ", na, na, " << outputArrays["hasSource"][index-1] << ", " << outputArrays["hasDest"][index-1] << ", na, na" << "\n";
      index++;
    } 
  }

 

  for(auto &id : arrayOrder)
  {
    bool first = true;
    
      outFile << id << " = [ ";
      for(auto const& i : outputArrays[id]) {
        if (first)
          outFile << i;
        else 
          outFile << ", " << i;
        first = false;
      }
       outFile << "];   \n";
  }

  //  std::ofstream outFileCle;
  // outFileCle.open ("init_cle.mzn");
  
  for(auto &id : nodeOrder)
  {
    for(auto const& i : outputEnumsPDGNode[id]) {
      Node* node = nodeID2Node[i];
      if(node->getAnno() != "None")
      {
        outFile <<  "constraint hasCle[" << nodeID2index[i] << "]=" << node->getAnno() << "; \n"; 
      }
    } 
  }

  
  // _PDG->dumpNodeLineNumbers();

  // outFileCle.close();
  outFile.close();
  dbgFile.close();
  node2line.close();
}


static RegisterPass<pdg::MiniZincPrinter>
    ZINCPRINTER("minizinc",
               "Dump PDG data in minizinc format",
               false, false);