#pragma once

#ifdef __GXX_RTTI
#include "mpl_string/string_template.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <istream>
#include <iostream>
namespace common
{
namespace storages
{
	namespace xml
	{		
		namespace detail
		{
			struct load_funct
			{
				load_funct(const boost::property_tree::ptree& set):set_(set){}
				template<typename type_t>		
				void operator()(type_t& item) const
				{
					using boost::property_tree::ptree;
                    boost::property_tree::ptree::const_assoc_iterator result = item.value();
                    boost::property_tree::ptree::const_assoc_iterator it = set_.find( item.key() );
					if( it != set_.not_found() )
					{
						ptree::value_type const& v = (*it);
						result = v.second.get<typename type_t::value_type>("");
					}
					item.load_value(result);
				}
			private:
				const boost::property_tree::ptree& set_;
			};
			struct save_funct
			{
				save_funct(boost::property_tree::ptree& set):set_(set){}
				template<typename type_t>		
				void operator()(const type_t& item) const
				{
					using boost::property_tree::ptree;
					const typename type_t::value_type &value = item.value();
					set_.put(item.key(),value);
				}
			private:
				boost::property_tree::ptree& set_;
			};
		}
		template<typename init_vector>
		struct xml_storage
		{
			typedef detail::load_funct load_struct;
            static std::string path_to_save()
			{
				typedef mpl_template_string::template_string<init_vector> path_to_save_t;
                return path_to_save_t::template_string_value_path();
			}
			template<typename fusion_t>
			static bool load(fusion_t &fuc);
			template<typename fusion_t>
			static bool save(const fusion_t &fuc);
		};

		template<typename init_vector>template<typename fusion_t>
		bool xml_storage<init_vector>::save(const fusion_t &fuc)
		{
            std::ofstream output(path_to_save().c_str());
			using boost::property_tree::ptree;
			if(output.is_open())
			{
				ptree pt;
				ptree &settings= pt.add("Settings","");
				boost::fusion::for_each(fuc, detail::save_funct(settings));	
				write_xml( output, pt );
				output.close();
			}
			return true;
		}

		template<typename init_vector>template<typename fusion_t>
		bool xml_storage<init_vector>::load(fusion_t &fuc)
		{
            std::ifstream input(path_to_save().c_str());
			using boost::property_tree::ptree;
			if(input.is_open())
			{
				ptree pt;
				read_xml(input, pt);
				try
				{
					const boost::property_tree::ptree& settings = pt.get_child("Settings");	
					boost::fusion::for_each(fuc, detail::load_funct(settings));				
				}
				catch (const boost::property_tree::ptree_bad_data& error)
				{
					std::cout << error.what() << std::endl;
				}
				catch (const boost::property_tree::ptree_bad_path& error)
				{
					std::cout << error.what() << std::endl;
				}
				input.close();
			}
			return true;
		}
	}
}
}
#endif
