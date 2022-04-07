#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char value[32];
  int result;
  
  
  

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char *str){
    
    WP *node = NULL;                  //  定义一个新指针
    node = free_;
    free_ = free_->next; 
    if(head != NULL){
      node->next = head;
    }else{
      node->next =NULL;
    }
    head=node;
    strcpy(head->value,str);

    bool a= false;
    head->result= expr(str,&a);
    // while(head!=NULL){
    //   printf("%sbiannli",head->value);
    //   head=head->next;
    // }
    

    
  
    
  return head;
}


void free_wp(int number){
   WP * temp = head;
   WP * forward = NULL; 
    while(temp != NULL &&temp->NO != number){
        forward =temp;
        temp=temp->next;
      }

     if(temp == NULL) printf("没有监视点，无法删除");  
     if(forward != NULL){
         forward->next = forward->next->next;//删除某个结点的方法就是更改前一个结点的指针域
      }else{
        head =temp->next;
      }
      
    
      temp->next = free_;
      free_ = temp;
    
    
}

//扫描监视点

  bool scanWP(){
    bool a= false;
    WP*tmp=head;
    if(tmp ==NULL) return true;
    while(tmp !=NULL){
      int end= expr(tmp->value,&a);
      if(end==tmp->result){
        return true;
      }else{
      tmp->result=end;
       return false; 
      }
      tmp=tmp->next;
    }

    return false;
  }

  //打印监视点
 void infoWP(){
    WP*tmp=head;
    while(tmp !=NULL){
       printf("监视点名称 %s",tmp->value);
       printf("监视点值 %x",tmp->result);
       tmp=tmp->next;
    }



 }


