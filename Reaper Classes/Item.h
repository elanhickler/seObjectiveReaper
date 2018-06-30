#pragma once

enum GroupMode { none, grouped, overlapping, touching };

class ITEM : public OBJECT_MOVABLE, public OBJECT_NAMABLE, public OBJECT_VALIDATES
{
  //static functions
public:
  static bool isGrouped(const ITEM & i1, const ITEM & i2, bool must_be_on_same_track = true);
  static ITEM get(int idx);
  static ITEM getSelected(int idx);
  static ITEM CreateMidi(MediaTrack * track, double position, double length);

  enum FADESHAPE
  {
    LIN,
    LOG,
    EXPO,
    LOG_2,
    EXPO_2,
    SCURV,
    SCURV_2
  };

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
  ITEM(MediaItem * item);
  ITEM(MediaItem_Take * take);

  // conversion
  operator void*() const { return item; }
  operator MediaItem*() const { return item; }
  operator MediaItem_Take*() const { return *getActiveTake(); }

  // operator
  bool operator==(const MediaItem * rhs) const { return item == rhs; }
  bool operator!=(const MediaItem * rhs) const { return item != rhs; }
  bool operator==(const ITEM & rhs) const { return item == rhs.item; }
  bool operator!=(const ITEM & rhs) const { return item != rhs.item; }
  TAKE operator[](int i) const { return GetTake(item, i); }

  /* FUNCTIONS */

  ITEM duplicate();
  void remove();
  void move(double v);
  bool crop(RANGE r, bool move_edge);

  // returns the item created from the right-hand-side of the split, or invalid item if split failed
  ITEM split(double v);

  // returns ITEMLIST of items created from splitting including itself
  ITEMLIST split(vector<double> splitlist);

  TAKELIST GetTakes();
  void CollectTakes(); 

  MediaItem* pointer() { return item; }

  /* GETTER */

  const TAKE * getActiveTake() const;
  TAKE * getActiveTake();
  const TAKE * getTake(int i) const;
  TAKE * getTake(int i);
  int getNumTakes(); 

  MediaTrack* getTrack() const;
  int getTrackIndex() const;

  bool isMuted() const;
  bool isSelected() const;

  int getIndex() const;  
  int getGroupIndex() const;  
  
  double getVolume() const;
  double getRate() const;
  double getSnapOffset() const;

  double getFadeInLen() const;
  double getFadeOutLen() const;
  double getFadeInLenAuto() const;
  double getFadeOutLenAuto() const;
  int getFadeInShape() const;
  int getFadeOutShape() const;
  double getFadeInCurve() const;
  double getFadeOutCurve() const; 

  /* SETTER */
  void setMuted(bool v);
  void setSelected(bool v);

  void setActiveTake(int idx);

  bool setTrack(MediaTrack* track);
  void setTrackByIndex(int v); 
  bool setTrackByName(String name);

  void setVolume(double v);
  void setRate(double new_rate, bool warp = true);  
  void setSnapOffset(double v);  
  
  void setFadeInLen(double v);
  void setFadeOutLen(double v);
  void setFadeInLenAuto(double v);
  void setFadeOutLenAuto(double v);
  void setFadeInShape(int v);
  void setFadeOutShape(int v);
  void setFadeInCurve(double v);
  void setFadeOutCurve(double v);

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
    r ={ i.getStartPosition(), i.getEndPosition() };
  }

  RANGE r;

  operator MediaItem*() const { return list[0]; }
  operator ITEM() const { return list[0]; }
  ITEMLIST operator=(vector<ITEM> rhs) { list = rhs; return *this; }

  void CollectItems();
  void CollectSelectedItems();

  // functions
  void InitAudio();
  void move(double v);
  void remove();
  int crop(RANGE r, bool move_edge);

  // getters
  double getStartPosition() const;
  double getEndPosition() const;
  double getLength() const;
  double getSnapOffset() const;
  double getFadeInLen() const;
  double getFadeOutLen() const;
  String GetPropertyStringFromKey(const String & key, bool use_value) const;
  bool isSelected() const;
  RANGE range() const { return r; }

  // setters
  int setTrack(MediaTrack* track);
  void setEndPosition(double v);
  void setSnapOffset(double v);
  void setSelected(bool select);
  
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