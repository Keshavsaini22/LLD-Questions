#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class FileSystem;
class Folder;

class FileSystemItem
{
    friend class FileSystem;

private:
    string id_;
    string name_;
    Folder *parent_;

protected:
    FileSystemItem(string id, string name, Folder *parent)
        : id_(move(id)), name_(move(name)), parent_(parent) {}

public:
    virtual ~FileSystemItem() = default;

    const string &getId() const { return id_; }
    const string &getName() const { return name_; }
    Folder *getParent() const { return parent_; }

    virtual uint64_t getSize() const = 0;
    virtual bool isFolder() const = 0;
};

class File final : public FileSystemItem
{
private:
    uint64_t sizeBytes_;

public:
    File(string id, string name, Folder *parent, uint64_t sizeBytes)
        : FileSystemItem(move(id), move(name), parent),
          sizeBytes_(sizeBytes) {}

    uint64_t getSize() const override
    {
        return sizeBytes_;
    }

    bool isFolder() const override
    {
        return false;
    }
};

class Folder final : public FileSystemItem
{
    friend class FileSystem;

private:
    map<string, unique_ptr<FileSystemItem>> children_;

public:
    Folder(string id, string name, Folder *parent)
        : FileSystemItem(move(id), move(name), parent) {}

    uint64_t getSize() const override
    {
        uint64_t total = 0;

        for (const auto &[name, child] : children_)
        {
            (void)name;
            total += child->getSize();
        }

        return total;
    }

    bool isFolder() const override
    {
        return true;
    }
};

struct ItemInfo
{
    string id;
    string name;
    bool folder;
    uint64_t sizeBytes;
};

class FileSystem
{
private:
    unique_ptr<Folder> root_;

    // Non-owning pointers. Actual ownership remains with unique_ptr.
    unordered_map<string, FileSystemItem *> index_;

    uint64_t nextId_ = 1;
    mutable shared_mutex mutex_;

    static void validateName(const string &name)
    {
        if (name.empty() || name == "." || name == ".." ||
            name.find('/') != string::npos ||
            name.find('\\') != string::npos)
        {
            throw invalid_argument("Invalid item name");
        }
    }

    string generateId()
    {
        return "ITEM-" + to_string(nextId_++);
    }

    FileSystemItem *findItem(const string &id) const
    {
        auto it = index_.find(id);

        if (it == index_.end())
        {
            throw runtime_error("Item not found: " + id);
        }

        return it->second;
    }

    Folder *findFolder(const string &id) const
    {
        auto *item = findItem(id);
        auto *folder = dynamic_cast<Folder *>(item);

        if (!folder)
        {
            throw invalid_argument("Target item is not a folder");
        }

        return folder;
    }

    static string buildPath(const FileSystemItem *item)
    {
        if (!item->getParent())
        {
            return "/";
        }

        vector<string> parts;

        for (auto *current = item;
             current && current->getParent();
             current = current->getParent())
        {
            parts.push_back(current->getName());
        }

        string path;

        for (auto it = parts.rbegin(); it != parts.rend(); ++it)
        {
            path += "/" + *it;
        }

        return path.empty() ? "/" : path;
    }

    static void collectSearch(
        const Folder *folder,
        const string &name,
        vector<string> &results)
    {
        for (const auto &[childName, child] : folder->children_)
        {
            (void)childName;

            if (child->getName() == name)
            {
                results.push_back(buildPath(child.get()));
            }

            if (auto *subfolder =
                    dynamic_cast<const Folder *>(child.get()))
            {
                collectSearch(subfolder, name, results);
            }
        }
    }

    void removeIndexRecursively(const FileSystemItem *item)
    {
        if (auto *folder =
                dynamic_cast<const Folder *>(item))
        {
            for (const auto &[name, child] : folder->children_)
            {
                (void)name;
                removeIndexRecursively(child.get());
            }
        }

        index_.erase(item->getId());
    }

public:
    FileSystem()
        : root_(make_unique<Folder>("ROOT", "", nullptr))
    {
        index_[root_->getId()] = root_.get();
    }

    string getRootId() const
    {
        return "ROOT";
    }

    string createFile(
        const string &parentId,
        const string &name,
        uint64_t sizeBytes)
    {
        validateName(name);

        unique_lock lock(mutex_);

        Folder *parent = findFolder(parentId);

        if (parent->children_.count(name))
        {
            throw runtime_error("Name already exists in folder");
        }

        string id = generateId();

        auto file =
            make_unique<File>(id, name, parent, sizeBytes);

        FileSystemItem *raw = file.get();

        parent->children_.emplace(name, move(file));
        index_[id] = raw;

        return id;
    }

    string createFolder(
        const string &parentId,
        const string &name)
    {
        validateName(name);

        unique_lock lock(mutex_);

        Folder *parent = findFolder(parentId);

        if (parent->children_.count(name))
        {
            throw runtime_error("Name already exists in folder");
        }

        string id = generateId();

        auto folder =
            make_unique<Folder>(id, name, parent);

        FileSystemItem *raw = folder.get();

        parent->children_.emplace(name, move(folder));
        index_[id] = raw;

        return id;
    }

    vector<ItemInfo> list(const string &folderId) const
    {
        shared_lock lock(mutex_);

        const Folder *folder =
            dynamic_cast<const Folder *>(findItem(folderId));

        if (!folder)
        {
            throw invalid_argument("Target item is not a folder");
        }

        vector<ItemInfo> result;
        result.reserve(folder->children_.size());

        for (const auto &[name, child] : folder->children_)
        {
            (void)name;

            result.push_back({child->getId(),
                              child->getName(),
                              child->isFolder(),
                              child->getSize()});
        }

        return result;
    }

    vector<string> searchByName(const string &name) const
    {
        shared_lock lock(mutex_);

        vector<string> results;
        collectSearch(root_.get(), name, results);

        return results;
    }

    string getPath(const string &id) const
    {
        shared_lock lock(mutex_);
        return buildPath(findItem(id));
    }

    uint64_t getSize(const string &id) const
    {
        shared_lock lock(mutex_);
        return findItem(id)->getSize();
    }

    void renameItem(
        const string &id,
        const string &newName)
    {
        validateName(newName);

        unique_lock lock(mutex_);

        FileSystemItem *item = findItem(id);

        if (item == root_.get())
        {
            throw invalid_argument("Cannot rename root");
        }

        Folder *parent = item->parent_;

        if (newName == item->name_)
        {
            return;
        }

        if (parent->children_.count(newName))
        {
            throw runtime_error("Name already exists in folder");
        }

        // Rename the map key without moving the owned object.
        auto node = parent->children_.extract(item->name_);

        node.key() = newName;
        item->name_ = newName;

        parent->children_.insert(move(node));
    }

    void moveItem(
        const string &itemId,
        const string &destinationFolderId)
    {
        unique_lock lock(mutex_);

        FileSystemItem *item = findItem(itemId);
        Folder *destination = findFolder(destinationFolderId);

        if (item == root_.get())
        {
            throw invalid_argument("Cannot move root");
        }

        Folder *source = item->parent_;

        if (source == destination)
        {
            return;
        }

        if (destination->children_.count(item->name_))
        {
            throw runtime_error(
                "Name already exists in destination folder");
        }

        // Prevent moving a folder into itself or its descendants.
        if (auto *movingFolder = dynamic_cast<Folder *>(item))
        {
            for (Folder *current = destination;
                 current;
                 current = current->parent_)
            {
                if (current == movingFolder)
                {
                    throw invalid_argument(
                        "Cannot move a folder into its descendant");
                }
            }
        }

        // Transfer ownership while preserving the item's address and ID.
        auto node = source->children_.extract(item->name_);

        node.mapped()->parent_ = destination;

        destination->children_.insert(move(node));
    }

    void deleteItem(const string &id)
    {
        unique_lock lock(mutex_);

        FileSystemItem *item = findItem(id);

        if (item == root_.get())
        {
            throw invalid_argument("Cannot delete root");
        }

        Folder *parent = item->parent_;

        // Remove IDs for the entire subtree before destroying it.
        removeIndexRecursively(item);

        // Erasing the unique_ptr destroys the item and its descendants.
        parent->children_.erase(item->name_);
    }
};

int main()
{
    try
    {
        FileSystem fs;

        const string root = fs.getRootId();

        string documents =
            fs.createFolder(root, "documents");

        string projects =
            fs.createFolder(root, "projects");

        string notes =
            fs.createFile(documents, "notes.txt", 1200);

        fs.createFile(projects, "report.pdf", 5000);

        string archive =
            fs.createFolder(projects, "archive");

        fs.createFile(archive, "old.txt", 800);

        // Rename and move a file.
        fs.renameItem(notes, "meeting-notes.txt");
        fs.moveItem(notes, projects);

        cout << "Path: " << fs.getPath(notes) << '\n';

        cout << "Projects size: "
             << fs.getSize(projects) << " bytes\n";

        cout << "\nSearch for report.pdf:\n";

        for (const string &path :
             fs.searchByName("report.pdf"))
        {
            cout << "  " << path << '\n';
        }

        cout << "\nRoot contents:\n";

        for (const auto &item : fs.list(root))
        {
            cout << (item.folder ? "[Folder] " : "[File]   ")
                 << item.name
                 << " (" << item.sizeBytes << " bytes)\n";
        }

        fs.deleteItem(archive);

        cout << "\nProjects size after deleting archive: "
             << fs.getSize(projects) << " bytes\n";
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
