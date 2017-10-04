#pragma once

namespace group_mode { enum { none, grouped, overlapping, touching  }; }

class ITEM : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
    //static functions
public:
    static bool is_grouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track = true);
    static ITEM get(int idx);
    static ITEM getSelected(int idx);

private:
    friend class ITEMLIST;
    friend class TagParser;
    // member
    MediaItem* item;
    TAKELIST TakeList;
    TAKE active_take;
    
    String getObjectName() const override;
    void setObjectName(const String & v) override;

    double getObjectStartPos() const override;
    void setObjectStartPos(double v) override;

    double getObjectLength() const override;
    void setObjectLength(double v) override;

    void setObjectPosition(double v) override;

    int getObjectColor() const override;
    void setObjectColor(int v) override;

    bool objectIsValid() const override;

public:
    // constructor
    ITEM();
    ITEM(int i) { item = GetMediaItem(0, i); }
    ITEM(MediaItem* item);
    ITEM(MediaItem_Take * take);

    // conversion
    operator void*() const { return item; }
    operator MediaItem*() const { return item; }
    operator MediaItem_Take*() const { return activeTake(); }

    // operator
    bool operator==(const MediaItem* rhs) const { return item == rhs; }
    bool operator!=(const MediaItem* rhs) const { return item != rhs; }
    bool operator==(const ITEM & rhs) const { return item == rhs.item; }
    bool operator!=(const ITEM & rhs) const { return item != rhs.item; }
    TAKE operator[](int i) const { return GetTake(item, i); }

    // function
    void remove();
    void split(double v);
    TAKELIST GetTakes();
    void CollectTakes();
    MediaItem* pointer() { return item; }

    // getter
    int idx() const;
    MediaTrack* track() const;
    int track_idx() const;
    double snap() const;
    bool mute() const;
    int group() const;
    double vol() const;
    double fadeinlen() const;
    double fadeoutlen() const;
    double fadeinlen_auto() const;
    double fadeoutlen_auto() const;
    int fadein_shape() const;
    int fadeout_shape() const;
    double fadein_curve() const;
    double fadeout_curve() const;
    bool selected() const;

    const TAKE & activeTake() const;
    const TAKE & take(int i) const;
    const TAKE & take() const;
    TAKE & activeTake();
    TAKE & take(int i);
    TAKE & take();
    int num_takes();
    double rate() const;
    ITEM duplicate();

    // setter
    void track_idx(int v);
    void track(int v);
    bool track(MediaTrack* track);
    bool track(String name);
    void activeTake(int idx);
    void snap(double v);
    void mute(bool v);
    void vol(double v);
    void move(double v);
    bool crop(RANGE r, bool move_edge);
    void fadeinlen(double v);
    void fadeoutlen(double v);
    void fadeinlen_auto(double v);
    void fadeoutlen_auto(double v);
    void fadein_shape(int v);
    void fadeout_shape(int v);
    void fadein_curve(double v);
    void fadeout_curve(double v);
    void selected(bool v);
    void rate(double new_rate, bool warp = true);

    //~ITEM() { jassert(false); };

private:
    enum
    {
        __name,
        __track,
        __length,
        __rate,
        __volume,
        __snapoffset,
        __position,
        __fadeinlen,
        __fadeoutlen,
        __startoffset,
        __tags,
        __pitch,
        __file_extension
    };

    map<String, int> method_lookup ={
        { "N", __name },
        { "T", __track },
        { "L", __length },
        { "R", __rate },
        { "V", __volume },
        { "SN", __snapoffset },
        { "PP", __position },
        { "FI", __fadeinlen },
        { "FO", __fadeoutlen },
        { "SO", __startoffset },
        { "t", __tags },
        { "ext", __file_extension }
    };

    String GetPropertyStringFromKey(const String & key, bool get_value = false) const override;
};

class ITEMLIST : public LIST<ITEM>
{
private:
    int m_group;

public:
    // constructor
    ITEMLIST() {}
    ITEMLIST(ITEM i) 
    { 
        push_back(i); 
        r = { i.startPos(), i.endPos() };
    }

    RANGE r;

    operator MediaItem*() const { return list[0]; }
    operator ITEM() const { return list[0]; }
    ITEMLIST operator=(vector<ITEM> rhs) { list = rhs; return *this; }

    void CollectItems();
    void CollectSelectedItems();

    // getters
    double startPos() const;
    double endPos() const;
    double length() const;
    double snap() const;
    double fadeinlen() const;
    double fadeoutlen() const;
    String GetPropertyStringFromKey(const String & key, bool use_value) const;
    bool selected() const;
    RANGE range() const { return r; }

    // setters
    int crop(RANGE r, bool move_edge);
    int track(MediaTrack* track);
    int track(String name);
    void endPos(double v);
    void snap(double v);
    void move(double v);
    void remove();
    void selected(bool select);

    // functions
    void InitAudio();
};

class ITEMGROUPLIST : public LIST<ITEMLIST>
{
private:
    //function
    void collect_donotgroup(bool selected_only);
    void collect_groupgrouped(bool selected_only);
    void collect_groupoverlapping(bool selected_only, bool must_be_overlapping);

public:
    // constructor
    ITEMGROUPLIST() {}
    ITEMGROUPLIST(ITEMLIST ItemList) { push_back(ItemList); }

    // project
    void CollectItems(int group_mode);
    void CollectSelectedItems(int group_mode);

    operator ITEMLIST() { return list[0]; }

    // mode 0 = do not group items
    // mode 1 = group grouped items
    // mode 2 = group overlapping items
    // mode 3 = group touching items

    // functions
    int countItems();
};
