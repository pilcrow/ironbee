//////////////////////////////////////////////////////////////////////////////
// Licensed to Qualys, Inc. (QUALYS) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.
// QUALYS licenses this file to You under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief IronBee - Radix Test Functions
/// 
/// @author Pablo Rincon <pablo.rincon.crespo@gmail.com>
//////////////////////////////////////////////////////////////////////////////

#include "ironbee_config_auto.h"

#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

#define TESTING

#include "util/util.c"
#include "util/radix.c"
#include "util/mpool.c"
#include "util/debug.c"

/* -- Helper functions -- */

/*
 * @internal
 * Helper functions for printing node info
 */
void padding(int i)
{
  int j = 0;
  for (j = 0; j < i; j++)
    printf("..");
}

/*
 * @internal
 * Helper functions for printing node info
 */
void printBin(uint8_t *prefix,
              uint8_t prefixlen)
{
  uint8_t i = 0;
  //printf("0x%x=", prefix);
  for (; i < prefixlen; i++) {
    if (i % 4 == 0)
      printf(" ");
    printf("%u", IB_READ_BIT(prefix[i / 8], i % 8) & 0x01);
  }
  printf(" [%d] ", prefixlen);
}

void pdata(void* d) {
    char *data = (char*)d;
    printf("%s", data);
}

/*
 * @internal
 * Helper functions for printing node info
 */
void printKey(ib_radix_prefix_t *prefix)
{
  if (prefix)
    printBin(prefix->rawbits, prefix->prefixlen);
}

/*
 * @internal
 * Helper function, prints user data recursively with indentation accumulated
 * from the tree level
 */
static void ib_radix_node_print_ud(ib_radix_t *radix,
                                   ib_radix_node_t *node,
                                   int level,
                                   int bitlen,
                                   uint8_t ud)
{

  int i = 0;
  printf("\n");

  padding(level);
  if (node->prefix == NULL)
    return;

  printKey(node->prefix);
  printf("KLen: %d", bitlen + (int)node->prefix->prefixlen);
  if (node->data) printf("[Y]");
  else printf("[N]");

  if (ud) {
    printf(" UD: ");
    if (node->data)
      radix->print_data(node->data);
    else {
      printf("Empty");
    }
  }

  if (node->zero != NULL)
    ib_radix_node_print_ud(radix, node->zero, level + 4,
                           bitlen + node->prefix->prefixlen, ud);

  if (node->one != NULL)
    ib_radix_node_print_ud(radix, node->one, level + 4,
                           bitlen + node->prefix->prefixlen, ud);
}

/*
 * @internal
 * Helper function, prints user data
 */
static void ib_radix_print(ib_radix_t *radix,
                    uint8_t ud)
{
  int level = 1;
  int i = 0;

  if (radix == NULL || radix->start == NULL) {
    printf("Empty\n");
    return;
  }
    
  padding(level);
  printf("ROOT: ");

  if (ud) {
    if (radix->start->data != NULL)
      radix->print_data(radix->start->data);
    else {
      printf("Empty");
    }
  }

  if (radix->start->zero)
    ib_radix_node_print_ud(radix, radix->start->zero,
                           level + 4, radix->start->prefix->prefixlen, ud);

  if (radix->start->one)
    ib_radix_node_print_ud(radix, radix->start->one,
                           level + 4, radix->start->prefix->prefixlen, ud);
  printf("\n");
}

/* -- Tests -- */

/// @test Test util radix library - ib_radix_new()
TEST(TestIBUtilRadix, test_radix_prefix_new)
{
    ib_mpool_t *mp = NULL;
    ib_radix_prefix_t *prefix = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_prefix_new(&prefix, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_prefix_new() failed - rc != IB_OK";

    ASSERT_TRUE(prefix->rawbits == NULL) << "ib_radix_prefix_new() failed - "
                                            "there should be no data";
    ASSERT_TRUE(prefix->prefixlen == 0) << "ib_radix_prefix_new() failed - "
                                    "wrong number of elements (should be zero)";

    ib_mpool_destroy(mp);
}

/// @test Test util radix library - ib_radix_prefix_create()
TEST(TestIBUtilRadix, test_radix_prefix_create_and_destroy)
{
    ib_mpool_t *mp = NULL;
    ib_radix_prefix_t *prefix = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";

    uint8_t *prefix_data = (uint8_t *) ib_mpool_calloc(mp, 1, 5);
    prefix_data[0] = 0xAA;
    prefix_data[1] = 0xBB;
    prefix_data[2] = 0xCC;
    prefix_data[3] = 0xDD;
    prefix_data[4] = 0xEE;
    
    rc = ib_radix_prefix_create(&prefix, prefix_data, 5 * 8, mp);

    ASSERT_TRUE(rc == IB_OK) << "ib_radix_prefix_create() failed - rc != IB_OK";
    ASSERT_TRUE(prefix->rawbits != NULL) << "ib_radix_prefix_create() failed - "
                                            "It should be pointing to key1";

    ASSERT_TRUE(prefix->rawbits[0] == 0xAA) << "ib_radix_prefix_create() failed"
                         " - It should be pointing to a byte with content 'h' ";
    ASSERT_TRUE(prefix->prefixlen == 5 * 8) << "ib_radix_prefix_create() failed"
                                   " - wrong prefix bit length (should be 4*8)";

    ib_mpool_destroy(mp);
}

/// @test Test util radix library - ib_radix_node_new()
TEST(TestIBUtilRadix, test_radix_node_new)
{
    ib_mpool_t *mp = NULL;
    ib_radix_node_t *node = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";
    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_node_new(&node, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_node_new() failed - rc != IB_OK";

    ASSERT_TRUE(node->zero == NULL) << "ib_radix_node_new() failed -"
                                       " this pointer should be null";
    ASSERT_TRUE(node->one == NULL) << "ib_radix_node_new() failed -"
                                      " this pointer should be null";

    ASSERT_TRUE(node->prefix == NULL) << "ib_radix_node_new() failed -"
                                         " this pointer should be null";
    ASSERT_TRUE(node->data == NULL) << "ib_radix_node_new() failed -"
                                   " wrong number of elements (should be zero)";

    ib_mpool_destroy(mp);
}

/// @test Test util radix library - ib_radix_new() and ib_radix_elements()
TEST(TestIBUtilRadix, test_radix_create_and_destroy)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";
    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, NULL, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed -"
                                         " wrong number of elements";

    ib_mpool_destroy(mp);
}

/// @test Test util radix library - ib_radix_new() and ib_radix_insert_data()
TEST(TestIBUtilRadix, test_radix_create_insert_destroy)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, NULL, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed -"
                                         " wrong number of elements";


    uint8_t *prefix_data = (uint8_t *) ib_mpool_calloc(mp, 1, 5);
    prefix_data[0] = 0xAA;
    prefix_data[1] = 0xBB;
    prefix_data[2] = 0xCC;
    prefix_data[3] = 0xDD;
    prefix_data[4] = 0xEE;
    
    ib_radix_prefix_t *prefix = NULL;
    rc = ib_radix_prefix_create(&prefix, prefix_data, 5 * 8, mp);

    ASSERT_TRUE(rc == IB_OK) << "ib_radix_prefix_create() failed - rc != IB_OK";

    rc = ib_radix_insert_data(radix, prefix, prefix_data);

    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";
    ASSERT_TRUE(radix->start != NULL) << "ib_radix_insert_data() failed -"
                                         " There should be a starting node";

    ASSERT_TRUE(radix->start->one != NULL) << "ib_radix_insert_data() failed -"
                                              " The prefix starts with bit 1 so"
                                              " start->one should hold a node "
                                              "with that prefix";
    ASSERT_TRUE(radix->start->zero == NULL) << "ib_radix_insert_data() failed -"
                                               " The prefix starts with bit 1 "
                                               "so start->zero should be null";

    prefix_data[0] = 0x0A;
    prefix_data[1] = 0xBB;
    prefix_data[2] = 0xCC;
    prefix_data[3] = 0xDD;
    prefix_data[4] = 0xEE;

    rc = ib_radix_insert_data(radix, prefix, prefix_data);

    ASSERT_TRUE(radix->start->zero != NULL) << "ib_radix_insert_data() failed -"
                                               " The prefix starts with bit 0 "
                                               "so start->zero should hold a "
                                               "node";

    ASSERT_TRUE(radix->start->one != NULL) << "ib_radix_insert_data() failed -"
                                              " The radix should also have a "
                                              "previous node at start->one";

    prefix_data[0] = 0x0A;
    prefix_data[1] = 0x0B;
    prefix_data[2] = 0xCC;
    prefix_data[3] = 0xDD;
    prefix_data[4] = 0xEE;

    rc = ib_radix_insert_data(radix, prefix, prefix_data);

    ASSERT_TRUE(radix->start->zero != NULL) << "ib_radix_insert_data() failed -"
                                               " The prefix starts with bit 0 "
                                               "so start->zero should hold a "
                                               "node";
    ASSERT_TRUE(radix->start->one != NULL) << "ib_radix_insert_data() failed - "
                                              "The radix should also have a "
                                              "previous node at start->one";

    ASSERT_TRUE(radix->start->zero->zero != NULL) << "ib_radix_insert_data() "
                "failed - The new prefix differs with the old one (the second "
                "inserted) in a 0 (at the second byte = 0x0B), "
                "so start->zero->zero should hold it";
    ASSERT_TRUE(radix->start->zero->one != NULL) << "ib_radix_insert_data() "
                "failed - The old (second) prefix differs with the new one by a"
                " bit 1 (at the second byte = 0xBB), so start->zero->one should"
                " hold a node with that prefix";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_new() and ib_radix_insert_data() 
 * with NULL datas */
TEST(TestIBUtilRadix, test_radix_insert_null_data)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, NULL, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";


    uint8_t *prefix_data = (uint8_t *) ib_mpool_calloc(mp, 1, 5);
    prefix_data[0] = 0xAA;
    prefix_data[1] = 0xBB;
    prefix_data[2] = 0xCC;
    prefix_data[3] = 0xDD;
    prefix_data[4] = 0xEE;
    
    ib_radix_prefix_t *prefix = NULL;
    rc = ib_radix_prefix_create(&prefix, prefix_data, 5 * 8, mp);

    ASSERT_TRUE(rc == IB_OK) << "ib_radix_prefix_create() failed - rc != IB_OK";

    rc = ib_radix_insert_data(radix, prefix, NULL);

    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";
    ASSERT_TRUE(radix->start != NULL) << "ib_radix_insert_data() failed - There"
                                         " should be a starting node";

    ASSERT_TRUE(radix->start->one != NULL) << "ib_radix_insert_data() failed - "
                "The prefix starts with bit 1 so start->one should hold a node "
                "with that prefix";
    ASSERT_TRUE(radix->start->zero == NULL) << "ib_radix_insert_data() failed -"
                " The prefix starts with bit 1 so start->zero should be null";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_ip_to_prefix() */
TEST(TestIBUtilRadix, test_radix_ip_to_prefix)
{
    ib_mpool_t *mp = NULL;
    ib_status_t rc;
    ib_radix_prefix_t *prefix = NULL;
    const char *ascii1 = "192.168.1.10";
    const char *ascii2 = "AAAA:BBBB::1";

    const char *ascii3 = "192.168.2.0/23";
    const char *ascii4 = "AAAA:BBBB::1/111";


    char *cidr1 = NULL;
    char *cidr2 = NULL;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";

    /* IPV4 prefix */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";

    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix() failed - rc != IB_OK";

    /* Check default prefix bit len */
    ASSERT_TRUE(prefix->prefixlen == 32) << "ib_radix_ip_to_prefix() failed - "
                                            "prefix len should be 32 since no "
                                            "mask was specified";

    /* IPV6 prefix */
    cidr2 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr2 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";

    memcpy(cidr2, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr2, &prefix, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix() failed - rc != IB_OK";

    /* Check default prefix bit len */
    ASSERT_TRUE(prefix->prefixlen == 128) << "ib_radix_ip_to_prefix() failed - "
                                            "prefix len should be 128 since no "
                                            "mask was specified";

    /* IPV4 prefix */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";

    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix() failed - rc != IB_OK";


    /* Check specified prefix bit len from CIDR */
    ASSERT_TRUE(prefix->prefixlen == 23) << "ib_radix_ip_to_prefix() failed - "
                                            "prefix len should be 23 (as cidr)";

    /* IPV6 prefix */
    cidr2 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr2 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    cidr2 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    memcpy(cidr2, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr2, &prefix, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix() failed - rc != IB_OK";

    /* Check specified prefix bit len from CIDR */
    ASSERT_TRUE(prefix->prefixlen == 111) << "ib_radix_ip_to_prefix() failed - "
                                           "prefix len should be 111 (as cidr)";

    ib_mpool_destroy(mp);
}

/* @test Test util radix library - ib_radix_match*() functions with ipv4 */
TEST(TestIBUtilRadix, test_radix_match_functions_ipv4)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    ib_list_t *results = NULL;
    char *result = NULL;

    ib_radix_prefix_t *prefix1 = NULL;
    ib_radix_prefix_t *prefix2 = NULL;
    ib_radix_prefix_t *prefix3 = NULL;
    ib_radix_prefix_t *prefix4 = NULL;
    ib_radix_prefix_t *prefix5 = NULL;

    const char *ascii1 = "192.168.1.1";
    const char *ascii2 = "192.168.1.10";
    const char *ascii3 = "192.168.0.0/16";
    const char *ascii4 = "10.0.0.1";
    const char *ascii5 = "192.168.1.27";

    char *cidr1 = NULL;

    ib_list_node_t *node;
    ib_list_node_t *node_next;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, pdata, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix1*/
    rc = ib_radix_insert_data(radix, prefix1, (void*)ascii1);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix2*/
    rc = ib_radix_insert_data(radix, prefix2, (void*)ascii2);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix3*/
    rc = ib_radix_insert_data(radix, prefix3, (void*)ascii3);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix4*/
    rc = ib_radix_insert_data(radix, prefix4, (void*)ascii4);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix5. We are not going to insert this one! */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii5) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii5, strlen(ascii5) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix5, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix5() failed - rc != IB_OK";

    ASSERT_TRUE(ib_radix_elements(radix) == 4) << "ib_radix_elements() failed -"
                                                  " there should be 4";

    /* Now that we have some keys inserted, let's test the matching functions */

    /* match all */
    rc = ib_radix_match_all_data(radix, prefix3, &results, mp);
    ASSERT_TRUE(results != NULL) << "results list should not be null";

    int i = 0;
    IB_LIST_LOOP_SAFE(results, node, node_next) {
        char *val = (char *)ib_list_node_data(node);
        ASSERT_TRUE(strcmp(val, ascii4) != 0) << "IB_LIST_LOOP_SAFE() failed"
                                                 " - wrong value";
        //printf("Elem: %s\n", val);
        i++;
    }   

    /* To view the tree contents -> ib_radix_print(radix, 1); */

    /* Now that we have some keys inserted, let's test the matching functions */
    ASSERT_TRUE(ib_list_elements(results) == 3) << "ib_radix_elements() failed"
                                                  " - there should be 3, got "
                                                  << ib_list_elements(results);

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix5, &result);
    ASSERT_TRUE(result == NULL) << "We did not insert this key, "
                                   "so it should be null";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix5, &result);
    ASSERT_TRUE(result != NULL) << "We did not insert this key, "
                                   "but closest is data of ascii3";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_match*() functions with ipv6 */
TEST(TestIBUtilRadix, test_radix_match_functions_ipv6)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    ib_list_t *results = NULL;
    char *result = NULL;

    ib_radix_prefix_t *prefix1 = NULL;
    ib_radix_prefix_t *prefix2 = NULL;
    ib_radix_prefix_t *prefix3 = NULL;
    ib_radix_prefix_t *prefix4 = NULL;
    ib_radix_prefix_t *prefix5 = NULL;

    const char *ascii1 = "AAAA:BBBB::1";
    const char *ascii2 = "AAAA:BBBB::12";
    const char *ascii3 = "AAAA:BBBB::0/64";
    const char *ascii4 = "FFFF:CCCC::1";
    const char *ascii5 = "AAAA:BBBB::27BC";

    char *cidr1 = NULL;

    ib_list_node_t *node;
    ib_list_node_t *node_next;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, pdata, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix1*/
    rc = ib_radix_insert_data(radix, prefix1, (void*)ascii1);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix2*/
    rc = ib_radix_insert_data(radix, prefix2, (void*)ascii2);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix3*/
    rc = ib_radix_insert_data(radix, prefix3, (void*)ascii3);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix4*/
    rc = ib_radix_insert_data(radix, prefix4, (void*)ascii4);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix5. We are not going to insert this one! */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii5) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii5, strlen(ascii5) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix5, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix5() failed - rc != IB_OK";

    ASSERT_TRUE(ib_radix_elements(radix) == 4) << "ib_radix_elements() failed -"
                                                  " there should be 4";

    /* Now that we have some keys inserted, let's test the matching functions */

    /* match all */
    rc = ib_radix_match_all_data(radix, prefix3, &results, mp);
    ASSERT_TRUE(results != NULL) << "results list should not be null";

    int i = 0;
    IB_LIST_LOOP_SAFE(results, node, node_next) {
        char *val = (char *)ib_list_node_data(node);
        ASSERT_TRUE(strcmp(val, ascii4) != 0) << "IB_LIST_LOOP_SAFE() failed"
                                                 " - wrong value";
        //printf("Elem: %s\n", val);
        i++;
    }   

    /* To view the tree contents -> ib_radix_print(radix, 1); */

    /* Now that we have some keys inserted, let's test the matching functions */
    ASSERT_TRUE(ib_list_elements(results) == 3) << "ib_radix_elements() failed"
                                                  " - there should be 3, got "
                                                  << ib_list_elements(results);

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix, prefix5, &result);
    ASSERT_TRUE(result == NULL) << "We did not insert this key, "
                                   "so it should be null";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix5, &result);
    ASSERT_TRUE(result != NULL) << "We did not insert this key, "
                                   "but closest is data of ascii3";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_match_closest() functions with ipv4*/
TEST(TestIBUtilRadix, test_radix_match_closest_ipv4)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    ib_list_t *results = NULL;
    char *result = NULL;

    ib_radix_prefix_t *prefix1 = NULL;
    ib_radix_prefix_t *prefix2 = NULL;
    ib_radix_prefix_t *prefix3 = NULL;
    ib_radix_prefix_t *prefix4 = NULL;
    ib_radix_prefix_t *prefix5 = NULL;

    const char *ascii1 = "10.0.1.0/24";
    const char *ascii_host1 = "10.0.1.4";
    const char *ascii2 = "10.0.0.0/24";
    const char *ascii_host2 = "10.0.0.127";
    const char *ascii3 = "10.0.0.0/16";
    const char *ascii_host3 = "10.0.14.240";
    const char *ascii4 = "10.0.0.0/8";
    const char *ascii_host4 = "10.127.14.240";
    const char *ascii5 = "192.168.1.1";

    char *cidr1 = NULL;

    ib_list_node_t *node;
    ib_list_node_t *node_next;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, pdata, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix1*/
    rc = ib_radix_insert_data(radix, prefix1, (void*)ascii1);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix2*/
    rc = ib_radix_insert_data(radix, prefix2, (void*)ascii2);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix3*/
    rc = ib_radix_insert_data(radix, prefix3, (void*)ascii3);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix4*/
    rc = ib_radix_insert_data(radix, prefix4, (void*)ascii4);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";


    /* The following prefixes are created only for queries,
     * (we are not going to insert them) */

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host1, strlen(ascii_host1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host2, strlen(ascii_host2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host3, strlen(ascii_host3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host4, strlen(ascii_host4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /* IPV4 prefix5 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii5) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii5, strlen(ascii5) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix5, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix5() failed - rc != IB_OK";

    ASSERT_TRUE(ib_radix_elements(radix) == 4) << "ib_radix_elements() failed -"
                                                  " there should be 4";

    /* Now that we have some keys inserted, let's test the matching functions */

    /* To view the tree contents -> ib_radix_print(radix, 1); */

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix1, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix1 that in fact is ascii_host1, but we didn't
     * insert it and the closest prefix is ascii1 (the smallest subnet with
     * data of ascii_host1) */
    ASSERT_TRUE(strcmp(result, ascii1) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix2 that in fact is ascii_host2, but we didn't
     * insert it and the closest prefix is ascii2 (the smallest subnet with
     * data of ascii_host2) */
    ASSERT_TRUE(strcmp(result, ascii2) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix3 that in fact is ascii_host3, but we didn't
     * insert it and the closest prefix is ascii3 (the smallest subnet with
     * data of ascii_host3) */
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix4 that in fact is ascii_host4, but we didn't
     * insert it and the closest prefix is ascii4 (the smallest subnet with
     * data of ascii_host4) */
    ASSERT_TRUE(strcmp(result, ascii4) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix5, &result);
    /* So we are searching prefix5 that in fact is ascii_host5, but we didn't
     * insert it and there's NO subnet inserted containing this host, so it
     * should return NULL */
    ASSERT_TRUE(result == NULL) << "results list should be null";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_match_closest() functions with ipv6*/
TEST(TestIBUtilRadix, test_radix_match_closest_ipv6)
{
    ib_mpool_t *mp = NULL;
    ib_radix_t *radix = NULL;
    ib_status_t rc;
    ib_list_t *results = NULL;
    char *result = NULL;

    ib_radix_prefix_t *prefix1 = NULL;
    ib_radix_prefix_t *prefix2 = NULL;
    ib_radix_prefix_t *prefix3 = NULL;
    ib_radix_prefix_t *prefix4 = NULL;
    ib_radix_prefix_t *prefix5 = NULL;

    const char *ascii1 = "AAAA:BBBB:CCCC:0000:0000:1:0:0/96";
    const char *ascii_host1 = "AAAA:BBBB:CCCC::1:0:4";
    const char *ascii2 = "AAAA:BBBB:CCCC:0000::/64";
    const char *ascii_host2 = "AAAA:BBBB:CCCC::1234:0000:1111:24CC";
    const char *ascii3 = "AAAA:BBBB::/32";
    const char *ascii_host3 = "AAAA:BBBB:ABCD:DDDD::1111:CCBA:2222";
    const char *ascii4 = "AAAA:BBBB:CCCC:0000:0000:DDDD:0000:AAAA/16";
    const char *ascii_host4 = "AAAA::CAFE";
    const char *ascii5 = "BBBB::1";

    char *cidr1 = NULL;

    ib_list_node_t *node;
    ib_list_node_t *node_next;
    
    atexit(ib_shutdown);
    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, pdata, NULL, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix1*/
    rc = ib_radix_insert_data(radix, prefix1, (void*)ascii1);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix2*/
    rc = ib_radix_insert_data(radix, prefix2, (void*)ascii2);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix3*/
    rc = ib_radix_insert_data(radix, prefix3, (void*)ascii3);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix4*/
    rc = ib_radix_insert_data(radix, prefix4, (void*)ascii4);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";


    /* The following prefixes are created only for queries,
     * (we are not going to insert them) */

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host1, strlen(ascii_host1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host2, strlen(ascii_host2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host3, strlen(ascii_host3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii_host4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii_host cidr";
    memcpy(cidr1, ascii_host4, strlen(ascii_host4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /* IPV4 prefix5 */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii5) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii5, strlen(ascii5) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix5, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix5() failed - rc != IB_OK";

    ASSERT_TRUE(ib_radix_elements(radix) == 4) << "ib_radix_elements() failed -"
                                                  " there should be 4";

    /* Now that we have some keys inserted, let's test the matching functions */

    /* To view the tree contents -> ib_radix_print(radix, 1); */

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix1, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix1 that in fact is ascii_host1, but we didn't
     * insert it and the closest prefix is ascii1 (the smallest subnet with
     * data of ascii_host1) */
    ASSERT_TRUE(strcmp(result, ascii1) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix2 that in fact is ascii_host2, but we didn't
     * insert it and the closest prefix is ascii2 (the smallest subnet with
     * data of ascii_host2) */
    ASSERT_TRUE(strcmp(result, ascii2) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix3 that in fact is ascii_host3, but we didn't
     * insert it and the closest prefix is ascii3 (the smallest subnet with
     * data of ascii_host3) */
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    /* So we are searching prefix4 that in fact is ascii_host4, but we didn't
     * insert it and the closest prefix is ascii4 (the smallest subnet with
     * data of ascii_host4) */
    ASSERT_TRUE(strcmp(result, ascii4) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix, prefix5, &result);
    /* So we are searching prefix5 that in fact is ascii_host5, but we didn't
     * insert it and there's NO subnet inserted containing this host, so it
     * should return NULL */
    ASSERT_TRUE(result == NULL) << "results list should be null";

    ib_mpool_destroy(radix->mp);
}

/* @test Test util radix library - ib_radix_clone() and matching
 * functions with ipv4 */
TEST(TestIBUtilRadix, test_radix_clone_and_match_functions_ipv4)
{
    ib_mpool_t *mp = NULL;
    ib_mpool_t *mp_tmp = NULL;

    ib_radix_t *radix = NULL;
    ib_radix_t *radix_ok = NULL;
    ib_status_t rc;
    ib_list_t *results = NULL;
    char *result = NULL;

    ib_radix_prefix_t *prefix1 = NULL;
    ib_radix_prefix_t *prefix2 = NULL;
    ib_radix_prefix_t *prefix3 = NULL;
    ib_radix_prefix_t *prefix4 = NULL;
    ib_radix_prefix_t *prefix5 = NULL;

    const char *ascii1 = "192.168.1.1";
    const char *ascii2 = "192.168.1.10";
    const char *ascii3 = "192.168.0.0/16";
    const char *ascii4 = "10.0.0.1";
    const char *ascii5 = "192.168.1.27";

    char *cidr1 = NULL;

    ib_list_node_t *node;
    ib_list_node_t *node_next;
    
    atexit(ib_shutdown);

    rc = ib_initialize();
    ASSERT_TRUE(rc == IB_OK) << "ib_initialize() failed - rc != IB_OK";

    rc = ib_mpool_create(&mp_tmp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_mpool_create(&mp, NULL);
    ASSERT_TRUE(rc == IB_OK) << "ib_mpool_create() failed - rc != IB_OK";
    
    rc = ib_radix_new(&radix, NULL, pdata, NULL, mp_tmp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_new() failed - rc != IB_OK";
    ASSERT_TRUE(radix != NULL) << "ib_radix_new() failed - NULL value";

    ASSERT_TRUE(ib_radix_elements(radix) == 0) << "ib_radix_new() failed -"
                                                  " wrong number of elements";
    ASSERT_TRUE(radix->start == NULL) << "ib_radix_new() failed - wrong number "
                                         "of elements";

    /* IPV4 prefix1 */
    cidr1 = (char *) ib_mpool_calloc(mp_tmp, 1, strlen(ascii1) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii1, strlen(ascii1) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix1, mp_tmp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix1() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix1*/
    rc = ib_radix_insert_data(radix, prefix1, (void*)ascii1);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix2 */
    cidr1 = (char *) ib_mpool_calloc(mp_tmp, 1, strlen(ascii2) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii2, strlen(ascii2) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix2, mp_tmp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix2() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix2*/
    rc = ib_radix_insert_data(radix, prefix2, (void*)ascii2);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix3 */
    cidr1 = (char *) ib_mpool_calloc(mp_tmp, 1, strlen(ascii3) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii3, strlen(ascii3) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix3, mp_tmp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix3() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix3*/
    rc = ib_radix_insert_data(radix, prefix3, (void*)ascii3);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix4 */
    cidr1 = (char *) ib_mpool_calloc(mp_tmp, 1, strlen(ascii4) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii4, strlen(ascii4) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix4, mp_tmp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix4() failed - rc != IB_OK";

    /*We are going to link it to the const ascii representation of the prefix4*/
    rc = ib_radix_insert_data(radix, prefix4, (void*)ascii4);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_insert_data() failed - rc != IB_OK";

    /* IPV4 prefix5. We are not going to insert this one */
    /* also we are going to use the definitive mem pool */
    cidr1 = (char *) ib_mpool_calloc(mp, 1, strlen(ascii5) + 1);
    ASSERT_TRUE(cidr1 != NULL) << "ib_mpool_calloc() failed - could not "
                                  "allocate mem for a ascii cidr";
    memcpy(cidr1, ascii5, strlen(ascii5) + 1);
    rc = ib_radix_ip_to_prefix(cidr1, &prefix5, mp);
    ASSERT_TRUE(rc == IB_OK) << "ib_radix_ip_to_prefix5() failed - rc != IB_OK";

    ASSERT_TRUE(ib_radix_elements(radix) == 4) << "ib_radix_elements() failed -"
                                                  " there should be 4";

    /* Now that we have some keys inserted, let's clone the radix to the other
     * memory pool (mp) */

    ib_radix_clone_radix(radix, &radix_ok, mp);

    /* destroy the temporary pool */
    ib_mpool_destroy(mp_tmp);

    /* Now let's test the matching functions */

    /* match all */
    rc = ib_radix_match_all_data(radix_ok, prefix3, &results, mp);
    ASSERT_TRUE(results != NULL) << "results list should not be null";

    int i = 0;
    IB_LIST_LOOP_SAFE(results, node, node_next) {
        char *val = (char *)ib_list_node_data(node);
        ASSERT_TRUE(strcmp(val, ascii4) != 0) << "IB_LIST_LOOP_SAFE() failed"
                                                 " - wrong value";
        //printf("Elem: %s\n", val);
        i++;
    }   

    /* To view the tree contents -> ib_radix_print(radix_ok, 1); */

    /* Now that we have some keys inserted, let's test the matching functions */
    ASSERT_TRUE(ib_list_elements(results) == 3) << "ib_radix_elements() failed"
                                                  " - there should be 3, got "
                                                  << ib_list_elements(results);

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix_ok, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix_ok, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix_ok, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) << "ib_radix_match_exact() failed"
                                             " - wrong result";

    /* match exact */
    result = NULL;
    rc = ib_radix_match_exact(radix_ok, prefix5, &result);
    ASSERT_TRUE(result == NULL) << "We did not insert this key, "
                                   "so it should be null";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix_ok, prefix2, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii2) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix_ok, prefix4, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii4) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix_ok, prefix3, &result);
    ASSERT_TRUE(result != NULL) << "results list should not be null";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    /* match closest */
    result = NULL;
    rc = ib_radix_match_closest(radix_ok, prefix5, &result);
    ASSERT_TRUE(result != NULL) << "We did not insert this key, "
                                   "but closest is data of ascii3";
    ASSERT_TRUE(strcmp(result, ascii3) == 0) <<"ib_radix_match_closest() failed"
                                             " - wrong result";

    ib_mpool_destroy(mp);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ib_trace_init(NULL);
    return RUN_ALL_TESTS();
}

