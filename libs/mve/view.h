/*
 * A reader, writer and API for MVE views.
 * Written by Simon Fuhrmann.
 *
 * MVE views are organized each in one directory, the directoy name being the
 * name of the view. The view contains one special file meta.ini and one
 * file for each embedding.
 *
 * Consider the following view representation in the file system:
 *
 * view_0000/
 *   meta.ini
 *   original.jpg
 *   undistorted.png
 *   normal_map.bin
 *   exif.bin
 *
 * The meta.ini file specifies basic view and camera information:
 *
 *   [view]
 *   id = 10
 *   name = DSC101983
 *
 *   [camera]
 *   focal_length = 0.812
 *   pixel_aspect = 1
 *   principal_point = 0.5 0.5
 *   rotation = 1 0 0 0 1 0 0 0 1
 *   translation = 0 0 0
 *
 * Altough JPEG files are allowed inside a view, saving a modified image will
 * always use a lossless format (PNG or MVEI), and the lossy file is deleted.
 * PNG is chosen for 1, 2, 3 and 4 channel images, MVEI for all others.
 *
 * TODO: File locks?
 */

#ifndef MVE_VIEW_HEADER
#define MVE_VIEW_HEADER

#include <stdint.h> // TODO: Use <cstdint> one C++11 is used.
#include <string>
#include <vector>
#include <map>

#include "util/ref_ptr.h"
#include "mve/defines.h"
#include "mve/camera.h"
#include "mve/image_base.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN

/**
 * File system representation of a MVE view.
 *
 * An MVE view is represented in a directory. This class manages the file
 * system layout, meta information in form of (key,value) pairs, and
 * dynamically loads images and blob.
 */
class View
{
public:
    typedef util::RefPtr<View> Ptr;
    typedef util::RefPtr<View const> ConstPtr;

    /**
     * View meta information that stores key/value pairs and the camera.
     * The key is the INI section name, a dot ".", and the key name.
     * The value is an arbitrary string (newlines are disallowed).
     */
    struct MetaData
    {
        MetaData (void);

        typedef std::map<std::string, std::string> KeyValueMap;
        CameraInfo camera;
        KeyValueMap data;
        bool is_dirty;
    };

    /** Proxy for images. */
    struct ImageProxy
    {
        ImageProxy (void);

        /* These fields are always initialized. */
        bool is_dirty;
        std::string name;
        std::string filename;
        /* These fields are initialized on-demand (get_*_proxy()). */
        bool is_initialized;
        int32_t width;
        int32_t height;
        int32_t channels;
        ImageType type;
        /* This field is initialized on request with get_image(). */
        ImageBase::Ptr image;
    };

    /** Proxy for BLOBs (Binary Large OBjects). */
    struct BlobProxy
    {
        BlobProxy (void);

        /* These fields are always initialized. */
        bool is_dirty;
        std::string name;
        std::string filename;
        /* These fields are initialized on-demand. */
        bool is_initialized;
        uint64_t size;
        /* This field is initialized on request with get_blob(). */
        ByteImage::Ptr blob;
    };

public:
    static View::Ptr create (void);
    static View::Ptr create (std::string const& path);

    /* --------------------- I/O interface -------------------- */

    /** Initializes the view from a directory. */
    void load_view (std::string const& path);

    /** Initializes the view from a deprecated .mve file. */
    void load_view_from_mve_file (std::string const& filename);

    /**
     * Reloads the view. This will discard all unsaved changes.
     */
    void reload_view (void);

    /** Writes the view to an MVE directory. */
    void save_view_as (std::string const& path);

    /** Saves dirty meta data, images and blobs, returns the amount saved. */
    int save_view (void);

    /** Returns the directory name the view is connected with. */
    std::string const& get_directory (void) const;

    /** Clears everything, discards potentially unsaved data. */
    void clear (void);

    /** Returns true if meta data, images or blobs are dirty. */
    bool is_dirty (void) const;

    /** Cleans unused data that is not dirty. Returns amount cleaned. */
    int cache_cleanup (void);

    /** Returns the memory consumption in bytes. */
    std::size_t get_byte_size (void) const;

    /* ---------------------- View Meta Data ---------------------- */

    /** Returns a value from the meta information. */
    std::string get_value (std::string const& key) const;

    /** Adds a key/value pair to the meta information. */
    void set_value (std::string const& key, std::string const& value);

    /** Deletes the key/value pair from the meta information. */
    void delete_value (std::string const& key);

    /** Sets the view ID (key "view.id"). */
    void set_id (int view_id);

    /** Sets the name of the view (key "view.name"). */
    void set_name (std::string const& name);

    /** Sets camera information of the view (section "camera"). */
    void set_camera (CameraInfo const& camera);

    /** Returns the ID of the view (key "view.id"). */
    int get_id (void) const;

    /** Returns the name of the view (key "view.name"). */
    std::string get_name (void) const;

    /** Returns the camera information of the view (section "camera"). */
    CameraInfo const& get_camera (void) const;

    /** Returns true if the camera is valid. */
    bool is_camera_valid (void) const;

    /* -------------------- Managing of images -------------------- */

    /** Initializes the proxy, loads and returns the image. */
    ImageBase::Ptr get_image (std::string const& name,
        ImageType type = IMAGE_TYPE_UNKNOWN);

    /** Returns an initialized image proxy by name. */
    ImageProxy const* get_image_proxy (std::string const& name,
        ImageType type = IMAGE_TYPE_UNKNOWN);

    /** Returns true if an image by that name exist. */
    bool has_image (std::string const& name,
        ImageType type = IMAGE_TYPE_UNKNOWN);

    /**
     * Adds a new image to the view.
     * If an image by that name already exists, an exception is raised.
     */
    void add_image (ImageBase::Ptr image, std::string const& name);

    /**
     * Sets an image to the view and marks it dirty.
     * If an image by that name already exists, it is overwritten.
     */
    void set_image (ImageBase::Ptr image, std::string const& name);

    /** Returns true if an image by that name has been removed. */
    bool remove_image (std::string const& name);

    /* --------------------- Managing of blobs -------------------- */

    /** Initializes the proxy, loads and returns the blob. */
    ByteImage::Ptr get_blob (std::string const& name);

    /** Returns an initialized blob proxy by name. */
    BlobProxy const* get_blob_proxy (std::string const& name);

    /** Returns true if a BLOB by that name exist. */
    bool has_blob (std::string const& name);

    /**
     * Adds a new BLOB to the view.
     * If a BLOB by that name already exists, an exception is raised.
     */
    void add_blob (ByteImage::Ptr blob, std::string const& name);

    /**
     * Sets a BLOB to the view and marks it dirty.
     * If an embedding by that name already exists, it is overwritten.
     */
    void set_blob (ByteImage::Ptr blob, std::string const& name);

    /** Returns true if a blob by that name has been removed. */
    bool remove_blob (std::string const& name);

    /* ----------------- Access to internal data ------------------ */

    /** Returns the view meta data. */
    MetaData const& get_meta_data (void) const;

    /** Returns a list of all image proxies (images may not be initialized). */
    std::vector<ImageProxy> const& get_images (void) const;

    /** Returns a list of all BLOB proxies (blobs may not be initialized). */
    std::vector<BlobProxy> const& get_blobs (void) const;

    /** Prints a formatted list of internal data. */
    void debug_print (void);

    /* ------------------ Convenience Functions ------------------- */

public: // TODO: Make protected.
    /** Creates an uninitialized view. */
    View (void);

    /** Creates a view and initializes from a directory. */
    View (std::string const& path);

private:
    void parse_meta_data_file (std::string const& fname);
    void load_meta_data (std::string const& fname);
    void save_meta_data (std::string const& fname);
    void populate_images_and_blobs (std::string const& path);
    void replace_file (std::string const& old_fn, std::string const& new_fn);

    ImageProxy* find_image_intern (std::string const& name);
    void initialize_image (ImageProxy* proxy, bool update);
    ImageBase::Ptr load_image (ImageProxy* proxy, bool update);
    void load_image_intern (ImageProxy* proxy, bool init_only);
    void save_image_intern (ImageProxy* proxy);

    BlobProxy* find_blob_intern (std::string const& name);
    void initialize_blob (BlobProxy* proxy, bool update);
    ByteImage::Ptr load_blob (BlobProxy* proxy, bool update);
    void load_blob_intern (BlobProxy* proxy, bool init_only);
    void save_blob_intern (BlobProxy* proxy);

protected:
    std::string path;
    MetaData meta_data;
    std::vector<ImageProxy> images;
    std::vector<BlobProxy> blobs;
};

/* ---------------------------------------------------------------- */

inline
View::MetaData::MetaData (void)
    : is_dirty(false)
{
}

inline
View::ImageProxy::ImageProxy (void)
    : is_dirty(false)
    , is_initialized(false)
    , width(0)
    , height(0)
    , channels(0)
    , type(IMAGE_TYPE_UNKNOWN)
{
}

inline
View::BlobProxy::BlobProxy (void)
    : is_dirty(false)
    , is_initialized(false)
    , size(0)
{
}

/* ---------------------------------------------------------------- */

inline
View::View (void)
{
}

inline
View::View (std::string const& path)
{
    this->load_view(path);
}

inline View::Ptr
View::create (void)
{
    return Ptr(new View());
}

inline View::Ptr
View::create (std::string const& path)
{
    return Ptr(new View(path));
}

inline std::string const&
View::get_directory (void) const
{
    return this->path;
}

inline View::MetaData const&
View::get_meta_data (void) const
{
    return this->meta_data;
}

inline std::vector<View::ImageProxy> const&
View::get_images (void) const
{
    return this->images;
}

inline std::vector<View::BlobProxy> const&
View::get_blobs (void) const
{
    return this->blobs;
}

inline void
View::set_id (int view_id)
{
    this->set_value("view.id", util::string::get(view_id));
}

inline void
View::set_name (std::string const& name)
{
    this->set_value("view.name", name);
}

inline int
View::get_id (void) const
{
    std::string value = this->get_value("view.id");
    return value.empty() ? -1 : util::string::convert<int>(value, true);
}

inline std::string
View::get_name (void) const
{
    return this->get_value("view.name");
}

inline CameraInfo const&
View::get_camera (void) const
{
    return this->meta_data.camera;
}

inline bool
View::is_camera_valid (void) const
{
    return this->meta_data.camera.flen > 0.0f;
}

MVE_NAMESPACE_END

#endif /* MVE_VIEW_HEADER */
