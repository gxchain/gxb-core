/*
 * Copyright (c) 2017 Peter Conrad, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/snapshot/snapshot.hpp>

#include <graphene/chain/database.hpp>

#include <fc/io/fstream.hpp>
#include <boost/algorithm/string.hpp>


using namespace graphene::snapshot_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

static const char* OPT_BLOCK_NUM  = "snapshot-at-block";
static const char* OPT_BLOCK_TIME = "snapshot-at-time";
static const char* OPT_DEST       = "snapshot-to";
static const char* OPT_OBJECT_ARRAY ="snapshot-objects";
static const char* OPT_OBJECT_COMP = "snapshot-objects-compilation";

void snapshot_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   command_line_options.add_options()
         (OPT_BLOCK_NUM, bpo::value<uint32_t>(), "Block number after which to do a snapshot")
         (OPT_BLOCK_TIME, bpo::value<string>(), "Block time (ISO format) after which to do a snapshot")
         (OPT_DEST, bpo::value<string>(), "Pathname of JSON file where to store the snapshot")
         (OPT_OBJECT_ARRAY, bpo::value<vector<string>>()->multitoken(), "The objects you want to export, Tuple of : space_id type_id object_id the * should be /* ")
         (OPT_OBJECT_COMP,bpo::value<string>()->composing(),"The objects you want to export")
         ;
   config_file_options.add(command_line_options);
}

std::string snapshot_plugin::plugin_name()const
{
   return "snapshot";
}

void snapshot_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("snapshot plugin: plugin_initialize() begin");

   if( options.count(OPT_BLOCK_NUM) || options.count(OPT_BLOCK_TIME) )
   {
      FC_ASSERT( options.count(OPT_DEST), "Must specify snapshot-to in addition to snapshot-at-block or snapshot-at-time!" );
      dest = options[OPT_DEST].as<std::string>();
      if( options.count(OPT_BLOCK_NUM) )
         snapshot_block = options[OPT_BLOCK_NUM].as<uint32_t>();
      if( options.count(OPT_BLOCK_TIME) )
         snapshot_time = fc::time_point_sec::from_iso_string( options[OPT_BLOCK_TIME].as<std::string>() );
      if( options.count(OPT_OBJECT_ARRAY) ){
         try
         {
            std::vector<std::string> snapshot_object_array = options[OPT_OBJECT_ARRAY].as<std::vector<std::string>>();
            snapshot_space_id = snapshot_object_array[0];
            snapshot_type_id  = snapshot_object_array[1];
            snapshot_object_id= snapshot_object_array[2];
         }
         catch ( fc::exception& e )
         {
             wlog("The snapshot-objects is malformed : ${ex}", ("ex",e.to_detail_string()));
         }
      }
      if( options.count(OPT_OBJECT_COMP) ){
         snapshot_objects_string = options[OPT_OBJECT_COMP].as<std::string>();
      }
      database().applied_block.connect( [&]( const graphene::chain::signed_block& b ) {
         check_snapshot( b );
      });
   }
   else
      FC_ASSERT( !options.count("snapshot-to"), "Must specify snapshot-at-block or snapshot-at-time in addition to snapshot-to!" );
   ilog("snapshot plugin: plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void snapshot_plugin::plugin_startup() {}

void snapshot_plugin::plugin_shutdown() {}

void snapshot_plugin::create_snapshot( const graphene::chain::database& db, const fc::path& dest )
{
   ilog("snapshot plugin: creating snapshot");
   fc::ofstream out;
   try
   {
      out.open( dest );
   }
   catch ( fc::exception& e )
   {
      wlog( "Failed to open snapshot destination: ${ex}", ("ex",e.to_detail_string()) );
      return;
   }

   if(snapshot_objects_string != ""){
      auto objects = fc::json::from_string(snapshot_objects_string).as<vector<string>>(2);
      for( const string& object : objects ){
         try{
            std::vector<std::string> split_string;
            boost::split(split_string, object, boost::is_any_of("."), boost::token_compress_on);
            auto space_id = std::stoi(split_string[0]);
            auto type_id = std::stoi(split_string[1]);
            auto& index = db.get_index( (uint8_t)space_id, (uint8_t)type_id);
            index.inspect_all_objects( [&out]( const graphene::db::object& o ) {
               out << fc::json::to_string( o.to_variant() ) <<  ","  << '\n';
            });
         } catch(fc::exception& e) {
            wlog("The snapshot-objects is malformed : ${ex}", ("ex",e.to_detail_string()) );
            continue;
         }
      }
   } else {
      uint32_t space_id_begin = 0;
      uint32_t type_id_begin = 0;
      uint32_t space_id_end = 0;
      uint32_t type_id_end = 0;
      uint32_t space_id_now =0;
      uint32_t type_id_now  =0;
      uint32_t object_id_now =0;

      int32_t object_id = -1;

      if(snapshot_space_id == "*"){
         space_id_end = 256;
         type_id_end  = 256;
      } else {
         try
         {
            space_id_now = std::stoi(snapshot_space_id);
         }
         catch (fc::exception& e)
         {
            wlog("The snapshot-objects is malformed : ${ex}", ("ex",e.to_detail_string()) );
         }
         space_id_begin = space_id_now;
         space_id_end = space_id_now;
         if(snapshot_type_id == "*"){
            type_id_end = 256;
         } else {
            try
            {
               type_id_now = std::stoi(snapshot_type_id);
            }
            catch(fc::exception& e)
            {
               wlog("The snapshot-objects is malformed : ${ex}", ("ex",e.to_detail_string()));
            }
            type_id_begin = type_id_now;
            type_id_end = type_id_now;
            if(snapshot_object_id == "*"){

            } else {
               try 
               {
                  const auto object_id_now = std::stoi(snapshot_object_id);
               }
               catch(fc::exception& e)
               {
               wlog("The snapshot-objects is malformed : ${ex}", ("ex",e.to_detail_string()) );
               }
               object_id = object_id_now;
            }
         }
      }  
      if(object_id == -1){
         for( ; space_id_begin <= space_id_end; space_id_begin++ ){
            for(uint32_t type_id_temp = type_id_begin; type_id_temp <= type_id_end; type_id_temp++)
            {
               try
               {
                  db.get_index( (uint8_t)space_id_begin, (uint8_t)type_id_temp );
               }
               catch (fc::assert_exception& e)
               {
                  continue;
               }
               auto& index = db.get_index( (uint8_t)space_id_begin, (uint8_t)type_id_temp );
               index.inspect_all_objects( [&out]( const graphene::db::object& o ) {
                  out << fc::json::to_string( o.to_variant() ) << "," << '\n';
               });
            }
         }
      } else {
            try
               {
                  db.get_index( (uint8_t)space_id_begin, (uint8_t)type_id_begin );
               }
               catch (fc::assert_exception& e)
               {
                  wlog("The snapshot-objects is malformed : ${ex}", ("ex", e.to_detail_string()));
               }
               auto& index = db.get_index( (uint8_t)space_id_begin, (uint8_t)type_id_begin );
               graphene::chain::object_id_type snapshot_object = {(uint8_t)space_id_begin, (uint8_t)type_id_begin, (uint64_t)object_id};
               auto index_object = index.find(snapshot_object);
               out << fc::json::to_string(index_object->to_variant()) << "," << '\n';
      }
   }
   out.close();
   ilog("snapshot plugin: created snapshot");
}

void snapshot_plugin::check_snapshot( const graphene::chain::signed_block& b )
{ try {
    uint32_t current_block = b.block_num();
    if( (last_block < snapshot_block && snapshot_block <= current_block)
           || (last_time < snapshot_time && snapshot_time <= b.timestamp) )
       create_snapshot( database(), dest );
    last_block = current_block;
    last_time = b.timestamp;
} FC_LOG_AND_RETHROW() }
