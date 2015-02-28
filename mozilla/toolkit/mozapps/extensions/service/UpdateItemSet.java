package org.mozilla.update.extensions;

public class UpdateItemSet
{
  private UpdateItem newestItem;
  private UpdateItem currentItem;

  public UpdateItemSet() 
  {
  }
  
  public UpdateItem getNewestItem()
  {
    return newestItem;
  }
  public void setNewestItem(UpdateItem newestItem)
  {
    this.newestItem = newestItem;
  }
  
  public UpdateItem getCurrentItem()
  {
    return currentItem;
  }
  public void setCurrentItem(UpdateItem currentItem)
  {
    this.currentItem = currentItem;
  }
}
