#include "clickhouse_config.h"

#if USE_YAML_CPP
#include "YAMLParser.h"

#include <vector>

#include <DBPoco/DOM/Document.h>
#include <DBPoco/DOM/NodeList.h>
#include <DBPoco/DOM/Element.h>
#include <DBPoco/DOM/AutoPtr.h>
#include <DBPoco/DOM/NamedNodeMap.h>
#include <DBPoco/DOM/Text.h>
#include <Common/Exception.h>

#error #include <yaml-cpp/yaml.h>

using namespace DBPoco::XML;

namespace DB
{

namespace ErrorCodes
{
    extern const int CANNOT_OPEN_FILE;
    extern const int CANNOT_PARSE_YAML;
}

namespace
{
    /// A prefix symbol in yaml key
    /// We add attributes to nodes by using a prefix symbol in the key part.
    /// Currently we use @ as a prefix symbol. Note, that @ is reserved
    /// by YAML standard, so we need to write a key-value pair like this: "@attribute": attr_value
    const char YAML_ATTRIBUTE_PREFIX = '@';

    DBPoco::AutoPtr<DBPoco::XML::Element> cloneXMLNode(const DBPoco::XML::Element & original_node)
    {
        DBPoco::AutoPtr<DBPoco::XML::Element> clone_node = original_node.ownerDocument()->createElement(original_node.nodeName());
        original_node.parentNode()->appendChild(clone_node);
        return clone_node;
    }

    void processNode(const YAML::Node & node, DBPoco::XML::Element & parent_xml_node)
    {
        auto * xml_document = parent_xml_node.ownerDocument();
        switch (node.Type())
        {
            case YAML::NodeType::Scalar:
            {
                std::string value = node.as<std::string>();
                DBPoco::AutoPtr<DBPoco::XML::Text> xml_value = xml_document->createTextNode(value);
                parent_xml_node.appendChild(xml_value);
                break;
            }

            /// For sequences we repeat the parent xml node. For example,
            /// seq:
            ///     - val1
            ///     - val2
            /// is converted into the following xml:
            /// <seq>val1</seq>
            /// <seq>val2</seq>
            ///
            /// A sequence of mappings is converted in the same way:
            /// seq:
            ///     - k1: val1
            ///       k2: val2
            ///     - k3: val3
            /// is converted into the following xml:
            /// <seq><k1>val1</k1><k2>val2</k2></seq>
            /// <seq><k3>val3</k3></seq>
            case YAML::NodeType::Sequence:
            {
                size_t i = 0;
                for (auto it = node.begin(); it != node.end(); ++it, ++i)
                {
                    const auto & child_node = *it;

                    bool need_clone_parent_xml_node = (i > 0);

                    if (need_clone_parent_xml_node)
                    {
                        /// Create a new parent node with same tag for each child node
                        processNode(child_node, *cloneXMLNode(parent_xml_node));
                    }
                    else
                    {
                        /// Map, so don't recreate the parent node but add directly
                        processNode(child_node, parent_xml_node);
                    }
                }
                break;
            }

            case YAML::NodeType::Map:
            {
                for (const auto & key_value_pair : node)
                {
                    const auto & key_node = key_value_pair.first;
                    const auto & value_node = key_value_pair.second;
                    std::string key = key_node.as<std::string>();
                    bool is_attribute = (key.starts_with(YAML_ATTRIBUTE_PREFIX) && value_node.IsScalar());
                    if (is_attribute)
                    {
                        /// we use substr(1) here to remove YAML_ATTRIBUTE_PREFIX from key
                        auto attribute_name = key.substr(1);
                        std::string value = value_node.as<std::string>();
                        parent_xml_node.setAttribute(attribute_name, value);
                    }
                    else
                    {
                        if (key == "#text" && value_node.IsScalar())
                        {
                            for (Node * child_node = parent_xml_node.firstChild(); child_node; child_node = child_node->nextSibling())
                                if (child_node->nodeType() == Node::TEXT_NODE)
                                    throw Exception(ErrorCodes::CANNOT_PARSE_YAML,
                                                    "YAMLParser has encountered node with several text nodes "
                                                    "and cannot continue parsing of the file");
                            std::string value = value_node.as<std::string>();
                            DBPoco::AutoPtr<DBPoco::XML::Text> xml_value = xml_document->createTextNode(value);
                            parent_xml_node.appendChild(xml_value);
                        }
                        else
                        {
                            DBPoco::AutoPtr<DBPoco::XML::Element> xml_key = xml_document->createElement(key);
                            parent_xml_node.appendChild(xml_key);
                            processNode(value_node, *xml_key);
                        }
                    }
                }
                break;
            }

            case YAML::NodeType::Null: break;
            case YAML::NodeType::Undefined:
            {
                throw Exception(ErrorCodes::CANNOT_PARSE_YAML,
                                "YAMLParser has encountered node with undefined type and cannot continue parsing "
                                "of the file");
            }
        }
    }
}


DBPoco::AutoPtr<DBPoco::XML::Document> YAMLParser::parse(const String& path)
{
    YAML::Node node_yml;
    try
    {
        node_yml = YAML::LoadFile(path);
    }
    catch (const YAML::ParserException& e)
    {
        /// yaml-cpp cannot parse the file because its contents are incorrect
        throw Exception(ErrorCodes::CANNOT_PARSE_YAML, "Unable to parse YAML configuration file {}, {}", path, e.what());
    }
    catch (const YAML::BadFile&)
    {
        /// yaml-cpp cannot open the file even though it exists
        throw Exception(ErrorCodes::CANNOT_OPEN_FILE, "Unable to open YAML configuration file {}", path);
    }
    DBPoco::AutoPtr<DBPoco::XML::Document> xml = new Document;
    DBPoco::AutoPtr<DBPoco::XML::Element> root_node = xml->createElement("clickhouse");
    xml->appendChild(root_node);
    try
    {
        processNode(node_yml, *root_node);
    }
    catch (const YAML::TypedBadConversion<std::string>&)
    {
        throw Exception(ErrorCodes::CANNOT_PARSE_YAML,
                        "YAMLParser has encountered node with key "
                        "or value which cannot be represented as string and cannot continue parsing of the file");
    }
    return xml;
}

}
#endif
