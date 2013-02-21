/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'string_util.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <ert/util/util.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <ert/util/string_util.h>


/*****************************************************************/

/* 
   This functions parses an input string 'range_string' of the type:

     "0,1,8, 10 - 20 , 15,17-21"
 
   I.e. integers separated by "," and "-". The integer values are
   parsed out. The result can be returned in two different ways:


    o If active != NULL the entries in active (corresponding to the
      values in the range) are marked as true. All other entries are
      marked as false. The active array must be allocated by the
      calling scope, with length (at least) "max_value + 1".
      
    o If active == NULL - an (int *) pointer is allocated, filled with
      the active indices and returned.

*/

//#include <stringlist.h>
//#include <tokenizer.h>
//static int * util_sscanf_active_range__NEW(const char * range_string , int max_value , bool * active , int * _list_length) {
//  tokenizer_type * tokenizer = tokenizer_alloc( NULL  , /* No ordinary split characters. */
//                                                NULL  , /* No quoters. */
//                                                ",-"  , /* Special split on ',' and '-' */
//                                                " \t" , /* Removing ' ' and '\t' */
//                                                NULL  , /* No comment */
//                                                NULL  );
//  stringlist_type * tokens;
//  tokens = tokenize_buffer( tokenizer , range_string , true);
//  
//  stringlist_free( tokens );
//  tokenizer_free( tokenizer );
//} 
   


static void  __add_item__(int **_active_list , int * _current_length , int *_list_length , int value) {
  int *active_list    = *_active_list;
  int  current_length = *_current_length;
  int  list_length    = *_list_length;

  active_list[current_length] = value;
  current_length++;
  if (current_length == list_length) {
    list_length *= 2;
    active_list  = util_realloc( active_list , list_length * sizeof * active_list );
    
    *_active_list = active_list;
    *_list_length = list_length;
  }
  *_current_length = current_length;
}



static int * util_sscanf_active_range__(const char * range_string , int * _list_length) {
  int *active_list    = NULL;
  int  current_length = 0;
  int  list_length;
  int  value,value1,value2;
  char  * start_ptr = (char *) range_string;
  char  * end_ptr;
  bool didnt_work = false;
  
  list_length = 10;
  active_list = util_calloc( list_length , sizeof * active_list );
  
    
  while (start_ptr != NULL) {
    value1 = strtol(start_ptr , &end_ptr , 10);
    
    if (end_ptr == start_ptr){
      printf("Returning to menu: %s \n" , start_ptr);
      didnt_work = true;
      break;
    }
    /* OK - we have found the first integer, now there are three possibilities:
       
      1. The string contains nothing more (except) possibly whitespace.
      2. The next characters are " , " - with more or less whitespace.
      3. The next characters are " - " - with more or less whitespace.
    
    Otherwise it is a an invalid string.
    */


    __add_item__(&active_list , &current_length , &list_length , value1);

    /* Skipping trailing whitespace. */
    start_ptr = end_ptr;
    while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
      start_ptr++;
    
    
    if (start_ptr[0] == '\0') /* We have found the end */
      start_ptr = NULL;
    else {
      /* OK - now we can point at "," or "-" - else malformed string. */
      if (start_ptr[0] == ',' || start_ptr[0] == '-') {
        if (start_ptr[0] == '-') {  /* This is a range */
          start_ptr++; /* Skipping the "-" */
          while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
            start_ptr++;
          
          if (start_ptr[0] == '\0') {
            /* The range just ended - without second value. */
            printf("%s[0]: malformed string: %s \n",__func__ , start_ptr);
            didnt_work = true; 
            break;
          }
          value2 = strtol(start_ptr , &end_ptr , 10);
          if (end_ptr == start_ptr) {
            printf("%s[1]: failed to parse integer from: %s \n",__func__ , start_ptr);
            didnt_work = true;
            break;
          }
          
          if (value2 < value1){
            printf("%s[2]: invalid interval - must have increasing range \n",__func__);
            didnt_work = true;
            break;
          }
          start_ptr = end_ptr;
          { 
            int value;
            for (value = value1 + 1; value <= value2; value++) 
              __add_item__(&active_list , &current_length , &list_length , value);
          }
          
          /* Skipping trailing whitespace. */
          while (start_ptr[0] != '\0' && isspace(start_ptr[0]))
            start_ptr++;
          
          
          if (start_ptr[0] == '\0')
            start_ptr = NULL; /* We are done */
          else {
            if (start_ptr[0] == ',')
              start_ptr++;
            else{
              printf("%s[3]: malformed string: %s \n",__func__ , start_ptr);
              didnt_work = true;
              break;
            }
          }
        } else 
          start_ptr++;  /* Skipping "," */

        /**
           When this loop is finished the start_ptr should point at a
           valid integer. I.e. for instance for the following input
           string:  "1-3 , 78"
                           ^
                           
           The start_ptr should point at "78".
        */

      } else{
        printf("%s[4]: malformed string: %s \n",__func__ , start_ptr);
        didnt_work = true;
        break;
      }
    }
  }
  if (_list_length != NULL)
    *_list_length = current_length;
  
  return active_list;
}



static int * util_sscanf_alloc_active_list(const char * range_string , int * list_length) {
  return util_sscanf_active_range__(range_string , list_length);
}



/*****************************************************************/

static bool_vector_type * alloc_mask( const int_vector_type * active_list ) {
  bool_vector_type * mask = bool_vector_alloc( 0 , false );
  int i;
  for (i=0; i < int_vector_size( active_list ); i++) 
    bool_vector_iset( mask , int_vector_iget( active_list , i) , true );

  return mask;
}



void string_util_update_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_sort( active_list );
  {
    bool_vector_type * mask = alloc_mask( active_list );
    string_util_update_active_mask( range_string , mask );

    int_vector_reset( active_list );
    {
      int i;
      for (i=0; i < bool_vector_size(mask); i++) {
        bool active = bool_vector_iget( mask , i );
        if (active)
          int_vector_append( active_list , i );
      }
    }
    
    bool_vector_free( mask );
  }
}


void string_util_init_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_reset( active_list );
  string_util_update_active_list( range_string , active_list );
}


int_vector_type *  string_util_alloc_active_list( const char * range_string ) {
  int_vector_type * active_list = int_vector_alloc( 0 , 0 );
  string_util_init_active_list( range_string , active_list );
  return active_list;
}

/*****************************************************************/

/*
  This is the only function which actually invokes the low level
  string parsing in util_sscanf_alloc_active_list().  
*/

void string_util_update_active_mask( const char * range_string , bool_vector_type * active_mask) {
  int length , i;
  int * sscanf_active = util_sscanf_alloc_active_list( range_string , &length);
  for (i=0; i < length; i++)
    bool_vector_iset( active_mask , sscanf_active[i] , true );
  
  util_safe_free( sscanf_active );
}


void string_util_init_active_mask( const char * range_string , bool_vector_type * active_mask ) {
  bool_vector_reset( active_mask );
  string_util_update_active_mask( range_string , active_mask );
}


bool_vector_type * string_util_alloc_active_mask( const char * range_string ) {
  bool_vector_type * mask  = bool_vector_alloc(0 , false );
  string_util_init_active_mask( range_string , mask );
  return mask;
}
