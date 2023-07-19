
#pragma once

#include <memory>


class Dom
{
public:
    Dom() {}
    Dom(const std::string& tag, const std::string& _class = "", const std::string& id = "")
        :tag_(tag),
        class_(_class),
        id_(id)
    {
    }

    void echo(std::string& buffer)
    {
        if (tag_.empty()) return;
        buffer.append("<" + tag_);
        if (!class_.empty())
        {
            buffer.append(" class=\"" + class_ + "\"");
        }
        if (!id_.empty())
        {
            buffer.append(" id=\"" + id_ + "\"");
        }
        if (!props_.empty())
        {
            for (auto& iter : props_)
            {
                if (iter.first.empty() || iter.second.empty()) continue;
                buffer.append(" " + iter.first + "=\"" + iter.second + "\"");
            }
        }
        buffer.append(">");
        if (!childs_.empty())
        {
            for (auto& child : childs_)
            {
                child.echo(buffer);
            }
        }
        if (!content_.empty())
        {
            buffer.append(content_);
        }
        buffer.append("</" + tag_ + ">");
    }

    Dom& child(const int pos)
    {
        if (pos >= childs_.size())
        {
            childs_.resize(pos);
        }
        return childs_[pos];
    }

    Dom& child(const std::string& tag, int idx = 0)
    {
        for (size_t i = 0; i < childs_.size(); i++)
        {
            if (childs_[i].tag_ == tag)
            {
                if (idx == 0)
                {
                    return childs_[i];
                }
                --idx;
            }
        }
        return create(tag);
    }

    Dom& create(const std::string& tag, const std::string& _class = "", const std::string& id = "")
    {
        childs_.resize(childs_.size() + 1);
        auto& dom = childs_.back();
        dom.tag_ = tag;
        if (!_class.empty())
            dom.class_ = _class;
        if (!id.empty())
            dom.id_ = id;
        return dom;
    }

    inline int size()
    {
        return (int)childs_.size();
    }

    inline std::string& operator[](const std::string& key)
    {
        return props_[key];
    }

    void operator=(const std::string& content) { content_ = content; }

    void toTable(std::vector<open::OpenCSV::CSVLine>& lines)
    {
        auto& table = create("table");
        while (!lines.empty() && lines[0].empty()) lines.erase(lines.begin());
        if (lines.empty())
        {
            return;
        }
        auto& l = lines[0];
        auto& tr = table.create("tr");
        int index = -1;
        for (int k = 0; k < l.size(); ++k)
        {
            if (l[k] == "code")
            {
                index = k;
            }
            tr.create("th") = l[k];
        }
        for (uint32_t i = 1; i < lines.size(); ++i)
        {
            auto& l = lines[i];
            if (l.empty()) continue;
            auto& tr = table.create("tr");

            for (int k = 0; k < l.size(); ++k)
            {
                if (index == k)
                {
                    tr.create("td") = "<a href = \"/cmd/stock/" + l[k] + "\" target=\"_blank\">" + l[k] + "</a>";
                }
                else
                {
                    tr.create("td") = l[k];
                }
            }
            //for (auto& item : l) tr.create("td") = item;
        }
    }

    static std::shared_ptr<Dom> DomCreate()
    {
        Dom* dom = new Dom("html");
        auto& head = dom->create("head");

        //auto& script = head.create("script");
        //script["type"] = "text/javascript";
        //script["src"] = "/file/tool.js";

        //auto& link = head.create("link");
        //link["rel"] = "stylesheet";
        //link["href"] = "/file/main.css";

        dom->create("body");
        auto sptr = std::shared_ptr<Dom>(dom);
        return sptr;
    }
protected:
    std::string tag_;
    std::string id_;
    std::string class_;
    std::string content_;
    std::vector<Dom> childs_;
    std::unordered_map<std::string, std::string> props_;
};

