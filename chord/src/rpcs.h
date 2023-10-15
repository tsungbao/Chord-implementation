#ifndef RPCS_H
#define RPCS_H
#include <deque>
#include <vector>
#include <cmath>
#include <string>
#include <queue>

#include "chord.h"

#include "rpc/client.h"

Node self, successor, predecessor;
std::deque<Node> successor_list ;
std::vector<Node> finger_table = std::vector<Node>(4);
int next = 0 ;  
bool joined_the_chord = false ; 


// We're not allowed to push self into successor list
// We'll delete the previous existing node n in our sl if it's duplicate
// with the incoming one
void push_front_successor_list(Node n){
  if(self.id != n.id){
    std::deque<Node> new_sl ;
    for(int i = 0 ; i < successor_list.size() ; i++){
      if(successor_list[i].id != n.id){
        new_sl.push_back(successor_list[i]);
      }
    }
    successor_list = new_sl ; 
    if(successor_list.size() > 4){
      successor_list.pop_back();
    }
    successor_list.push_front(n) ; 
  }
}

// Not allow to have self in our successor_list
void reconcile_sl_with_successor(std::deque<Node>& sl_of_successor){
  std::deque<Node> new_sl ;
  for(int i = 0 ; i < sl_of_successor.size() ; i++){
    if(sl_of_successor[i].id != self.id){
      new_sl.push_back(sl_of_successor[i]);
    }
  }
  successor_list = new_sl ; 
  if(successor.id != self.id){
    if(successor_list.size() > 4){
      successor_list.pop_back();
    }
    successor_list.push_front(successor) ; 
  }
}

bool id_in_sequence(uint64_t first , uint64_t sec , uint64_t third){
  if(first < sec && sec < third){
    return true ; 
  }
  // In a id space from 0~63
  if(first > third && sec < third){
     // 50     3       2     3
     return true ;
  }
  if(first > third && sec > first){
    //  50   3        63     50
    return true ; 
  }
  return false ; 
}

void join(Node n) {
  joined_the_chord = true ; 
  predecessor.ip = "";
  rpc::client client(n.ip, n.port);
  successor = client.call("find_successor", self.id).as<Node>();
  push_front_successor_list(successor) ;
}

Node get_info() { return self; } // Do not modify this line.
Node get_predecessor(){ return predecessor ; }
std::deque<Node> get_sucessor_list() { return successor_list ; }

// n think that it's my predecessor
void notify(Node n){
  if(predecessor.ip.compare("") == 0 || 
            id_in_sequence(predecessor.id , n.id , self.id)){
    predecessor = n ;
  }
}

// return the list of Nodes that have smaller id than the incoming
// id. The list will be sorted by its id from large to small.
// return self if there's no apt answer
std::vector<Node> closet_preceding_node(uint64_t id){
  
  // finger table: [smaller id -> larger id]
  // successor list: [smaller id -> larger id]

  int finger_table_start = -1 ;
  int successor_list_start = -1 ; 
  std::vector<Node> return_val ; 
  // O(logN) elements in the finger table
  for(int i = 3 ; i >= 0 ;i--){
    if(finger_table[i].ip.compare("")!=0 &&
              id_in_sequence(self.id,finger_table[i].id,id)){
      finger_table_start = i ; 
      break ; 
    }
  }
  // At this point finger_table_start points to the 
  // closet preceding node of input id in the finger table
  int n = successor_list.size() ; 
  for(int i = n - 1 ; i >= 0 ;i--){
    if(successor_list[i].ip.compare("")!=0 && 
            id_in_sequence(self.id,successor_list[i].id,id)){
      successor_list_start = i ;
      break ; 
    }
  }
  // At this point successor_list_start points to the 
  // closet preceding node of input id in the successor list 


  // Sort of like the last step of merge sort
  // We're going to put elements whose id are smaller than input id
  // into return_val in sequence. 
  //           id large  ->   small
  // return_val: [x x x x .... x]
  while(finger_table_start >=0 && successor_list_start >= 0){
    if(finger_table[finger_table_start].ip.compare("") == 0){
      finger_table_start-- ; 
      continue;
    }
    if(successor_list[successor_list_start].ip.compare("") == 0){
      successor_list_start-- ; 
      continue;
    }
    if(finger_table[finger_table_start].id 
          > successor_list[successor_list_start].id){
      return_val.push_back(finger_table[finger_table_start]) ;
      finger_table_start-- ; 
    }else{
      return_val.push_back(successor_list[successor_list_start]);
      successor_list_start-- ;
    }
  }

  while(finger_table_start >= 0){
    if(finger_table[finger_table_start].ip.compare("") != 0){
      return_val.push_back(finger_table[finger_table_start]) ;
    }    
    finger_table_start-- ; 
  }
  while(successor_list_start >= 0){
    if(successor_list[successor_list_start].ip.compare("") != 0){
      return_val.push_back(successor_list[successor_list_start]);
    }
    successor_list_start-- ;
  }

  if(return_val.size() > 0){
    return return_val ; 
  }
  return_val.push_back(self) ; 
  return return_val; 
}

Node find_successor(uint64_t id) {
  // TODO: implement your `find_successor` RPC
  if(successor.id == self.id){
    return self ; 
  }
  if(id_in_sequence(self.id,id,successor.id)){
    return successor ; 
  }

  //        id large  ->   small
  // cpn_list: [x x x x .... x]
  std::vector<Node> cpn_list = closet_preceding_node(id) ;

  if(cpn_list.size() == 1 && cpn_list[0].id == self.id){
    return self ; 
  }

  // At this point, it's impossible for cpn_list to have only one
  // element, which is self.
  Node return_val ; 
  for(int i = 0 ; i < cpn_list.size() ; i++){
    try{
      rpc::client client(cpn_list[i].ip, cpn_list[i].port) ; 
      return_val = client.call("find_successor",id).as<Node>() ; 
    }catch(std::exception& e){
      // The node we called may leave the chord
      // Try out the next option 
      continue ; 
    }
    break ;
  }

  return return_val ; 
}

void check_predecessor() {
  if(!joined_the_chord){
    return ; 
  }
  try {
    rpc::client client(predecessor.ip, predecessor.port);
    Node n = client.call("get_info").as<Node>();
  } catch (std::exception &e) {
    predecessor.ip = "";
  }
}

void fix_finger(){
  if(!joined_the_chord){
    return ; 
  }
  if(next >= 4){
    next = 0 ;
  }
  finger_table[next] = find_successor(
        (self.id+(uint64_t)pow(2,next+28)) % (uint64_t)(pow(2,32))  ) ;
  next++ ; 
}

void stabilize(){
  if(!joined_the_chord){
    if(predecessor.ip.compare("")!=0){
      if(self.id == successor.id){
        // This is a special case
        // From the perspective of this node,
        // there's only one node, which is
        // myself, in this chord. Thus, whoever join this chord 
        // (I sense it from my predecessor cuz it'll "notify" me)
        // , no matter how large or how small its id is, I'm 
        // gonna point my successor pointer to it
        successor = predecessor ; 
        successor_list.clear();
        push_front_successor_list(successor) ; 
      }
      joined_the_chord = true ; 
    }else{
      return ; 
    }
  }

  Node x ;
  try{
    rpc::client client(successor.ip, successor.port);
    x = client.call("get_predecessor").as<Node>();
  }catch(std::exception& e){
    // successor has failed
    successor_list.pop_front();
    if(successor_list.size()==0){
      if(predecessor.ip.compare("")!=0){
        // From my perspective, predecessor is the only node that 
        // I know in this chord
        // std::cout << self.id << "think only predecessor is alive\n!!!!" ;
        successor = predecessor ; 
        successor_list.clear();
        push_front_successor_list(successor) ;
      }else{
        // From my perspective, I'm the only node in the chord
        joined_the_chord = false ; 
        // std::cout << self.id << "think it's the only node in the chord!!!\n" ;
        successor_list.clear();
        successor = self ; 
      } 
      return ; 
    }
    successor = successor_list.front();
    stabilize();
    return ; 
  }
  // x is successor's predecessor
  if(x.ip.compare("")!=0 && id_in_sequence(self.id,x.id,successor.id)){
    successor = x ;
    if(successor_list.size() >= 4){
      successor_list.pop_back();
    }
    push_front_successor_list(x) ;
  }

  std::deque<Node> sl_of_successor ; 
  bool no_successor = false ; // either unable to connect with 
                              // the successor or successor is 
                              // the node itself (newly joined node)
  while(true){
    if(successor.id == self.id){
      no_successor = true ;
      break ; 
    }
    if(successor_list.size()<=0){
      no_successor = true ;
      break ; 
    }
    try{
      rpc::client client(successor.ip, successor.port);
      sl_of_successor = client.call("get_sucessor_list")
                        .as<std::deque<Node>>() ; 
      // successfully called the successor
      break ;
    }catch(std::exception& e){
      // successor has failed
      // At this point successor_list.size() is guaranteed to be
      // larger than 0
      successor_list.pop_front();
      successor = successor_list.front();
    }
  }

  if(no_successor){
    return ; 
  }
  reconcile_sl_with_successor(sl_of_successor) ; 

  // notify successor
  while(true){
    if(successor.id == self.id){
      no_successor = true ;
      break ; 
    }
    if(successor_list.size()<=0){
      no_successor = true ;
      break ; 
    }
    try{
      rpc::client client(successor.ip, successor.port);
      client.call("notify",self) ; 
      // successfully called the successor
      break ;
    }catch(std::exception& e){
      // successor has failed
      // At this point successor_list.size() is guaranteed to be
      // larger than 0
      successor_list.pop_front();
      successor = successor_list.front();
    }
  }
}

void create() {
  predecessor.ip = "";
  successor = self;
}

// void print_finger_table(){
//   std::cout << self.id << std::endl ; 
//   std::cout << "===finger table===" << std::endl ;
//   for(int i = 0 ; i <= 4 ; i++){
//     std::cout << finger_table[i].id <<std::endl ; 
//   }
//   std::cout << "===finger table===" << std::endl ;
//   std::cout << std::endl ; 
// }

// void print_successor_and_sl(){
//   std::cout << self.id << "'s successor = " << successor.id 
//             << " predecessor = " ;
//   if(predecessor.ip.compare("")==0){
//     std::cout << "None" ; 
//   }else{
//     std::cout << predecessor.id ; 
//   }
//   std::cout << std::endl ; 
//   std::cout << self.id << "'s successor list:" ;
//     for(int i = 0 ; i < successor_list.size() ; i++){
//       std::cout << successor_list[i].id << " " ; 
//     }
//   std::cout << std::endl ;
// }




void register_rpcs() {
  add_rpc("get_info", &get_info); // Do not modify this line.
  add_rpc("get_predecessor",&get_predecessor) ;
  add_rpc("get_sucessor_list",&get_sucessor_list) ; 
  add_rpc("notify",&notify) ; 
  add_rpc("create", &create);
  add_rpc("join", &join);
  add_rpc("find_successor", &find_successor);

  
  // add_rpc("print_finger_table", &print_finger_table);
  // add_rpc("print_successor_and_sl", &print_successor_and_sl);
}

void register_periodics() {
  add_periodic(check_predecessor);
  add_periodic(stabilize);
  add_periodic(fix_finger);
}

#endif /* RPCS_H */
