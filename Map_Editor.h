// ============================================================================
// Map Editor Definitions
// Programmed by Francois Lamini
// ============================================================================

#include "..\Code_Helper\Codeloader.hpp"
#include "..\Code_Helper\Allegro.hpp"

namespace Codeloader {

  class cLayout {

    public:
      cHash<std::string, tObject> components;
      std::string sel_component;
      std::string clicked;
      int width;
      int height;
      int cell_w;
      int cell_h;
      int red;
      int green;
      int blue;
      char** grid;
      cIO_Control* io;
      sPoint mouse_coords;
      bool not_clicked;

      cLayout(std::string name, std::string config, cIO_Control* io);
      ~cLayout();
      void Clear_Grid();
      void Parse_Grid(cFile& file);
      void Parse_Layout(std::string name);
      bool Has_Entity();
      void Parse_Entity();
      void Parse_Box(tObject& entity);
      void Parse_Field(tObject& entity);
      void Parse_Panel(tObject& entity);
      void Parse_Button(tObject& entity);
      void Parse_Properties(cFile& file);
      void Render();
      virtual void On_Component_Init(tObject& entity);
      virtual void On_Component_Render(tObject& entity);
      sRectangle Get_Entity_Dimensions(tObject& entity);
      bool Is_Identifier(char letter);
      virtual void On_Init();

  };

  class cMap_Editor : public cLayout {
    
    public:
      cHash<std::string, tObject> catalog;
      cHash<std::string, tObject_List> sprite_layers;
      tObject meta_data;
      std::string sel_layer;
      int sel_sprite;
      std::string sel_sprite_id;

      cMap_Editor(std::string name, std::string config, cIO_Control* io);
      void On_Component_Init(tObject& entity);
      void On_Component_Render(tObject& entity);
      void On_Init();
      void Load_Catalog(std::string name);
      void Load_Map(std::string name);
      void Save_Map(std::string name);
      void Init_Field(tObject& entity);
      void Render_Field(tObject& entity);
      void Init_Grid_View(tObject& entity);
      void Render_Grid_View(tObject& entity);
      void Render_Label(tObject& entity);
      void Init_List(tObject& entity);
      void Render_List(tObject& entity);
      void Render_Button(tObject& entity);
      void On_Button_Click(tObject& entity);
      void Init_Toolbar(tObject& entity);
      void Render_Toolbar(tObject& entity);
      void Init_Map_Editor(tObject& entity);
      void Render_Map_Editor(tObject& entity);
      void On_List_Click(tObject& entity, std::string text);
      void On_Toolbar_Click(tObject& entity, std::string label);
      void Load_Object_From_Grid_View(tObject& object, tObject& grid_view);
      void Save_Object_To_Grid_View(tObject& object, tObject& grid_view);
      void Update_Sprite_Palette(tObject& toolbar);
      void Scroll_Component(tObject& entity, sSignal& signal);
      void Update_Levels(tObject& list);
      void Select_Sprite(sSignal& signal, tObject& map_editor);
      sRectangle Parse_Rectangle(std::string text);
      void Render_Sprites(tObject& map_editor);
      void Clear_Map();
      void Destar_Sprite(tObject& sprite);

  };

}
