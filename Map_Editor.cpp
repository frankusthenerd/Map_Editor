// ============================================================================
// Map Editor Program
// Programmed by Francois Lamini
// ============================================================================
#include <iostream>
#include <string>

#include "Map_Editor.h"

bool Layout_Process();
bool Process_Keys();

// **************************************************************************
// Program Entry Point
// **************************************************************************

int main(int argc, char** argv) {
  if (argc == 2) {
    std::string map_name = argv[1];
    try {
      Codeloader::cConfig config("Config");
      int width = config.Get_Property("width");
      int height = config.Get_Property("height");
      Codeloader::cAllegro_IO allegro(map_name, width, height, 1, "Game");
      allegro.Load_Resources("Resources");
      allegro.Process_Messages(Layout_Process, Process_Keys);
    }
    catch (Codeloader::cError error) {
      error.Print();
    }
  }
  else {
    std::cout << "Usage: " << argv[0] << " <program>" << std::endl;
  }
  std::cout << "Done." << std::endl;
  return 0;
}

// ****************************************************************************
// Layout Processor
// ****************************************************************************

/**
 * Called when command needs to be processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Layout_Process() {
  return false;
}

/**
 * Called when keys are processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Process_Keys() {
  return false;
}

namespace Codeloader {

  // **************************************************************************
  // Layout Implementation
  // **************************************************************************

  /**
   * Creates a new layout module.
   * @param name The name of the layout file.
   * @param config The name of the config file.
   * @param io The I/O control.
   * @throws An error if the layout could not be loaded.
   */
  cLayout::cLayout(std::string name, std::string config, cIO_Control* io) {
    this->io = io;
    // Load the config file.
    cConfig layout_config(config + ".txt");
    this->width = layout_config.Get_Property("width");
    this->height = layout_config.Get_Property("height");
    this->cell_w = layout_config.Get_Property("cell-w");
    this->cell_h = layout_config.Get_Property("cell-h");
    this->red = layout_config.Get_Property("red");
    this->green = layout_config.Get_Property("green");
    this->blue = layout_config.Get_Property("blue");
    this->mouse_coords.x = 0;
    this->mouse_coords.y = 0;
    this->not_clicked = true;
    // Recalculate dimensions to grid dimensions.
    this->width /= this->cell_w;
    this->height /= this->cell_h;
    // Create the grid.
    this->grid = new char* [this->height];
    for (int row_index = 0; row_index < this->height; row_index++) {
      this->grid[row_index] = new char[this->width];
      for (int col_index = 0; col_index < this->width; col_index++) {
        this->grid[row_index][col_index] = ' ';
      }
    }
    // Load the layout.
    this->Parse_Layout(name);
  }

  /**
   * Frees the layout module.
   */
  cLayout::~cLayout() {
    for (int row_index = 0; row_index < this->height; row_index++) {
      delete[] this->grid[row_index];
    }
    delete[] this->grid;
  }

  /**
   * Clears out the grid.
   */
  void cLayout::Clear_Grid() {
    for (int row_index = 0; row_index < this->height; row_index++) {
      for (int col_index = 0; col_index < this->width; col_index++) {
        this->grid[row_index][col_index] = ' ';
      }
    }
  }

  /**
   * Parses the grid given the layout.
   * @param file The instance of the layout file.
   * @throws An error if the grid could not be loaded.
   */
  void cLayout::Parse_Grid(cFile& file) {
    for (int row_index = 0; row_index < this->height; row_index++) {
      std::string line = file.Get_Line();
      int col_count = (line.length() > this->width) ? this->width : line.length();
      for (int col_index = 0; col_index < col_count; col_index++) {
        char letter = line[col_index];
        this->grid[row_index][col_index] = letter;
      }
    }
  }

  /**
   * Parses the layout.
   * @param name The name of the layout.
   * @throws An error if something went wrong.
   */
  void cLayout::Parse_Layout(std::string name) {
    cFile layout_file(name + ".txt");
    layout_file.Read();
    // Parse grid first.
    this->Parse_Grid(layout_file);
    // Parse the entities.
    while (this->Has_Entity()) {
      this->Parse_Entity();
    }
    // Parse the properties here!
    this->Parse_Properties(layout_file);
    // Run all component initializers.
    int comp_count = this->components.Count();
    for (int comp_index = 0; comp_index < comp_count; comp_index++) {
      this->On_Component_Init(this->components.values[comp_index]);
    }
    // Run layout initializer.
    this->On_Init();
  }

  /**
   * Checks to see if there is an entity to parse.
   * @return True if there is an entity, false otherwise.
   */
  bool cLayout::Has_Entity() {
    bool has_entity = false;
    for (int cell_y = 0; cell_y < this->height; cell_y++) {
      for (int cell_x = 0; cell_x < this->width; cell_x++) {
        char cell = this->grid[cell_y][cell_x];
        if ((cell == '[') || (cell == '{') || (cell == '(') || (cell == '+')) { // Entity identifier.
          has_entity = true;
          break;
        }
      }
    }
    return has_entity;
  }

  /**
   * Parses an entity and adds it to the component stack.
   * @throws An error if the entity is invalid.
   */
  void cLayout::Parse_Entity() {
    tObject entity;
    entity["id"] = cValue("");
    entity["type"] = cValue("");
    entity["x"] = cValue(0);
    entity["y"] = cValue(0);
    entity["width"] = cValue(0);
    entity["height"] = cValue(0);
    for (int cell_y = 0; cell_y < this->height; cell_y++) {
      for (int cell_x = 0; cell_x < this->width; cell_x++) {
        char cell = this->grid[cell_y][cell_x];
        if (cell == '+') {
          entity["x"] = cell_x;
          entity["y"] = cell_y;
          entity["width"] = 1;
          entity["height"] = 1;
          entity["type"] = "box";
          this->Parse_Box(entity);
          // Break out of double loop.
          cell_y = this->height;
          break;
        }
        else if (cell == '[') {
          entity["x"] = cell_x;
          entity["y"] = cell_y;
          entity["width"] = 1;
          entity["height"] = 1;
          entity["type"] = "field";
          this->Parse_Field(entity);
          // Break out of double loop.
          cell_y = this->height;
          break;
        }
        else if (cell == '{') {
          entity["x"] = cell_x;
          entity["y"] = cell_y;
          entity["width"] = 1;
          entity["height"] = 1;
          entity["type"] = "panel";
          this->Parse_Panel(entity);
          // Break out of double loop.
          cell_y = this->height;
          break;
        }
        else if (cell == '(') {
          entity["x"] = cell_x;
          entity["y"] = cell_y;
          entity["width"] = 1;
          entity["height"] = 1;
          entity["type"] = "button";
          this->Parse_Button(entity);
          // Break out of double loop.
          cell_y = this->height;
          break;
        }
        else {
          continue; // Ignore but allow looking for other entities.
        }
      }
    }
    // Add to components.
    this->components[entity["id"].string] = entity;
  }

  /**
   * Parses a box structure.
   * @param entity The associated entity.
   * @throws An error if the box is invalid.
   */
  void cLayout::Parse_Box(tObject& entity) {
    // We'll navigate in this path: right -> down -> left -> up
    int pos_x = entity["x"].number; // Skip the plus.
    int pos_y = entity["y"].number;
    int rev_width = 1;
    int rev_height = 1;
    std::string id_str = "";
    // Clear out first plus.
    this->grid[pos_y][pos_x] = ' ';
    // Navigate right.
    pos_x++;
    while (pos_x < this->width) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == '+') {
        entity["width"].number++;
        entity["id"].string = id_str;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (this->Is_Identifier(cell)) { // Box Edge
        if (this->Is_Identifier(cell)) {
          id_str += cell; // Collect ID letter.
        }
        entity["width"].number++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid box. (right)");
      }
      pos_x++;
    }
    // Check for truncated object.
    if (pos_x == this->width) {
      throw cError("Truncated box. (width)");
    }
    // Navigate down.
    pos_y++; // Skip the first plus.
    while (pos_y < this->height) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == '+') {
        entity["height"].number++;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (cell == '|') {
        entity["height"].number++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid box. (down)");
      }
      pos_y++;
    }
    // Check for truncated object.
    if (pos_y == this->height) {
      throw cError("Truncated box. (height)");
    }
    // Navigate left.
    pos_x--; // Skip that first plus.
    while (pos_x >= 0) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == '+') {
        rev_width++;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (cell == '-') {
        rev_width++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid box. (left)");
      }
      pos_x--;
    }
    if (rev_width != entity["width"].number) {
      throw cError("Not a valid box. (width mismatch)");
    }
    // Navigate up.
    pos_y--;
    while (pos_y >= 0) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == ' ') { // Plus was removed but validated before.
        rev_height++;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (cell == '|') {
        rev_height++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid box. (up)");
      }
      pos_y--;
    }
    if (rev_height != entity["height"].number) {
      throw cError("Not a valid box. (height mismatch)");
    }
  }

  /**
   * Parses a field entity.
   * @param entity The associated entity.
   * @throws An error if the field is invalid.
   */
  void cLayout::Parse_Field(tObject& entity) {
    int pos_x = entity["x"].number;
    int pos_y = entity["y"].number;
    std::string id_str = "";
    // Clear out initial bracket.
    this->grid[pos_y][pos_x] = ' ';
    // Parse out field.
    pos_x++; // Pass over initial bracket.
    while (pos_x < this->width) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == ']') {
        entity["width"].number++;
        entity["id"].string = id_str;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (this->Is_Identifier(cell) || (cell == ' ')) {
        if (this->Is_Identifier(cell)) {
          id_str += cell;
        }
        entity["width"].number++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid field.");
      }
      pos_x++;
    }
    // Check for truncated object.
    if (pos_x == this->width) {
      throw cError("Truncated field.");
    }
  }

  /**
   * Parses a panel entity.
   * @param entity The associated entity.
   * @throws An error if the panel is invalid.
   */
  void cLayout::Parse_Panel(tObject& entity) {
    int pos_x = entity["x"].number;
    int pos_y = entity["y"].number;
    std::string id_str = "";
    // Clear out initial curly.
    this->grid[pos_y][pos_x] = ' ';
    // Skip over initial curly.
    pos_x++;
    // Go ahead and parse the rest.
    while (pos_x < this->width) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == '}') {
        entity["width"].number++;
        entity["id"].string = id_str;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (this->Is_Identifier(cell) || (cell == ' ')) {
        if (this->Is_Identifier(cell)) {
          id_str += cell;
        }
        entity["width"].number++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid panel.");
      }
      pos_x++;
    }
    // Check for truncated object.
    if (pos_x == this->width) {
      throw cError("Truncated panel.");
    }
  }

  /**
   * Parses a button entity.
   * @param entity The associated entity.
   * @throws An error if the button is invalid.
   */
  void cLayout::Parse_Button(tObject& entity) {
    int pos_x = entity["x"].number;
    int pos_y = entity["y"].number;
    std::string id_str = "";
    this->grid[pos_y][pos_x] = ' ';
    pos_x++;
    while (pos_x < this->width) {
      char cell = this->grid[pos_y][pos_x];
      if (cell == ')') {
        entity["width"].number++;
        entity["id"].string = id_str;
        this->grid[pos_y][pos_x] = ' ';
        break;
      }
      else if (this->Is_Identifier(cell) || (cell == ' ')) {
        if (this->Is_Identifier(cell)) {
          id_str += cell;
        }
        entity["width"].number++;
        this->grid[pos_y][pos_x] = ' ';
      }
      else {
        throw cError("Not a valid button.");
      }
      pos_x++;
    }
    // Check for truncated object.
    if (pos_x == this->width) {
      throw cError("Truncated button.");
    }
  }

  /**
   * Parses the properties associated with the layout.
   * @param file The instance of the layout file.
   * @throws An error if there is a problem with the layout.
   */
  void cLayout::Parse_Properties(cFile& file) {
    while (file.Has_More_Lines()) {
      std::string line = file.Get_Line();
      cArray<std::string> pair = Parse_Sausage_Text(line, "->");
      if (pair.Count() == 2) { // Property signature.
        std::string entity_id = pair[0];
        std::string value = pair[1];
        if (this->components.Does_Key_Exist(entity_id)) {
          cArray<std::string> props = Parse_Sausage_Text(value, ",");
          int prop_count = props.Count();
          for (int prop_index = 0; prop_index < prop_count; prop_index++) {
            cArray<std::string> prop = Parse_Sausage_Text(props[prop_index], "=");
            if (prop.Count() == 2) {
              std::string name = prop[0];
              std::string value = prop[1];
              try {
                this->components[entity_id][name] = cValue(Text_To_Number(value));
              }
              catch (cError number_error) { // A string.
                this->components[entity_id][name] = cValue(value);
              }
            }
            else {
              throw cError("Property is missing value.");
            }
          }
        }
        else {
          throw cError("Entity " + entity_id + " is not defined.");
        }
      }
      else {
        throw cError("Entity ID is missing properties.");
      }
    }
  }

  /**
   * Renders the entities.
   */
  void cLayout::Render() {
    // Render a background color.
    this->io->Color(this->red, this->green, this->blue);
    int entity_count = this->components.Count();
    this->clicked = "";
    for (int entity_index = 0; entity_index < entity_count; entity_index++) {
      tObject& entity = this->components.values[entity_index];
      // Read a mouse signal.
      sSignal signal = this->io->Read_Signal();
      if (signal.code == eSIGNAL_MOUSE) {
        sRectangle bump_map = this->Get_Entity_Dimensions(entity);
        if (Is_Point_In_Box(signal.coords, bump_map) && ((signal.button == eBUTTON_LEFT) || (signal.button == eBUTTON_RIGHT)) && this->not_clicked) { // Input focus.
          this->sel_component = this->components.keys[entity_index];
          this->clicked = this->components.keys[entity_index];
          // Normalize mouse coordinates to entity space.
          this->mouse_coords.x = signal.coords.x - entity["x"].number;
          this->mouse_coords.y = signal.coords.y - entity["y"].number;
          this->not_clicked = false;
        }
        else if (signal.button == eBUTTON_UP) {
          this->not_clicked = true;
        }
      }
      this->On_Component_Render(this->components.values[entity_index]);
    }
    // Render the screen.
    this->io->Refresh();
  }

  /**
   * Called when the component is initialized.
   * @param entity The associated entity.
   */
  void cLayout::On_Component_Init(tObject& entity) {
    // To be implemented in app.
  }

  /**
   * Called when the component is rendered.
   * @param entity The associated entity.
   */
  void cLayout::On_Component_Render(tObject& entity) {
    // To be implemented in app.
  }

  /**
   * Gets the dimensions of an entity.
   * @param entity The entity.
   * @return The rectangle with the dimensions.
   */
  sRectangle cLayout::Get_Entity_Dimensions(tObject& entity) {
    sRectangle dimensions;
    dimensions.left = entity["x"].number;
    dimensions.top = entity["y"].number;
    dimensions.right = dimensions.left + entity["width"].number - 1;
    dimensions.bottom = dimensions.top + entity["height"].number - 1;
    return dimensions;
  }

  /**
   * Determines if a letter is part of an identifier.
   * @param letter The letter to test.
   * @return True if the letter is an identifier, false if not.
   */
  bool cLayout::Is_Identifier(char letter) {
    return (((letter >= 'a') && (letter <= 'z')) || ((letter >= 'A') && (letter <= 'Z')) || ((letter >= '0') && (letter <= '9')) || (letter == '_'));
  }

  /**
   * Called when layout is initialized.
   */
  void cLayout::On_Init() {
    // To be implemented in the app.
  }

  // **************************************************************************
  // Map Editor Implementation
  // **************************************************************************

  /**
   * Creates a new map editor.
   * @param name The name of the layout file.
   * @param config The config file name.
   * @param io The I/O control.
   */
  cMap_Editor::cMap_Editor(std::string name, std::string config, cIO_Control* io) : cLayout(name, config, io) {
    this->sprite_layers["background"] = tObject_List();
    this->sprite_layers["platform"] = tObject_List();
    this->sprite_layers["character"] = tObject_List();
    this->sprite_layers["foreground"] = tObject_List();
    this->sprite_layers["overlay"] = tObject_List();
    this->meta_data["background"].Set_String("");
    this->meta_data["music"].Set_String("");
    this->sel_layer = "background";
    this->sel_sprite = NO_VALUE_FOUND;
    Check_Condition(this->components.Does_Key_Exist("layer"), "No layer field.");
    this->components["layer"]["text"].Set_String(this->sel_layer);
  }

  /**
   * Called when a component is initialized.
   * @param entity The entity that is initialized.
   */
  void cMap_Editor::On_Component_Init(tObject& entity) {
    if (entity["id"].string == "field") {
      this->Init_Field(entity);
    }
    else if (entity["id"].string == "grid-view") {
      this->Init_Grid_View(entity);
    }
    else if (entity["id"].string == "list") {
      this->Init_List(entity);
    }
    else if (entity["id"].string == "toolbar") {
      this->Init_Toolbar(entity);
    }
    else if (entity["id"].string == "map-editor") {
      this->Init_Map_Editor(entity);
    }
  }

  /**
   * Called when a component needs to be rendered.
   * @param entity The entity to be rendered.
   */
  void cMap_Editor::On_Component_Render(tObject& entity) {
    // Clear out the component. Make background white.
    this->io->Color(255, 255, 255);
    if (entity["id"].string == "field") {
      this->Render_Field(entity);
    }
    else if (entity["id"].string == "label") {
      this->Render_Label(entity);
    }
    else if (entity["id"].string == "grid-view") {
      this->Render_Grid_View(entity);
    }
    else if (entity["id"].string == "button") {
      this->Render_Button(entity);
    }
    else if (entity["id"].string == "toolbar") {
      this->Render_Toolbar(entity);
    }
    else if (entity["id"].string == "list") {
      this->Render_List(entity);
    }
    else if (entity["id"].string == "map-editor") {
      this->Render_Map_Editor(entity);
    }
    // Draw the canvas of the component.
    this->io->Draw_Canvas(entity["x"].number * this->cell_w, entity["y"].number * this->cell_h, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h);
  }

  /**
   * Called when the map editor is initialized.
   */
  void cMap_Editor::On_Init() {
    this->io->Set_Canvas_Target(); // We need to render to canvas for clipping.
  }

  /**
   * Loads the catalog from a file.
   * @param name The name of the file to load.
   * @throws An error if there was something wrong.
   */
  void cMap_Editor::Load_Catalog(std::string name) {
    cFile catalog_file(name + ".txt");
    catalog_file.Read();
    while (catalog_file.Has_More_Lines()) {
      std::string sprite_name = catalog_file.Get_Line();
      tObject sprite;
      catalog_file >>= sprite;
      this->Destar_Sprite(sprite);
      this->catalog[sprite_name] = sprite;
    }
    if (this->catalog.Count() > 0) { // Select the first catalog key.
      this->sel_sprite_id = this->catalog.keys[0];
    }
    else {
      throw cError("No sprites in catalog!");
    }
  }

  /**
   * Loads a map from a file.
   * @param name The name of the file to load.
   * @throws An error if the map could not be loaded.
   */
  void cMap_Editor::Load_Map(std::string name) {
    cFile map_file(name + ".map");
    map_file.Read();
    this->Clear_Map();
    map_file >>= this->meta_data;
    Check_Condition(this->meta_data.Does_Key_Exist("background"), "No background property in meta data.");
    Check_Condition(this->meta_data.Does_Key_Exist("music"), "No music property in meta data.");
    while (map_file.Has_More_Lines()) {
      tObject sprite;
      map_file >>= sprite;
      Check_Condition(sprite.Does_Key_Exist("layer"), "No layer property in sprite.");
      Check_Condition(this->sprite_layers.Does_Key_Exist(sprite["layer"].string), "Trying to load sprite to non-existant layer " + sprite["layer"].string + ".");
      this->sprite_layers[sprite["layer"].string].Add(sprite); // Add sprite to respective layer.
    }
    // Set fields.
    Check_Condition(this->components.Does_Key_Exist("level_name"), "No level name field.");
    Check_Condition(this->components.Does_Key_Exist("background"), "No background field.");
    Check_Condition(this->components.Does_Key_Exist("music"), "No music field.");
    this->components["level_name"]["text"].Set_String(name);
    this->components["background"]["text"].Set_String(this->meta_data["background"].string);
    this->components["music"]["text"].Set_String(this->meta_data["music"].string);
  }

  /**
   * Saves a map to a file.
   * @param name The name of the file to save to.
   * @throws An error if the map could not be saved.
   */
  void cMap_Editor::Save_Map(std::string name) {
    cFile map_file(name + ".map");
    map_file.Add(this->meta_data);
    int layer_count = this->sprite_layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      std::string name = this->sprite_layers.keys[layer_index];
      tObject_List& sprites = this->sprite_layers[name];
      int sprite_count = sprites.Count();
      for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
        tObject& sprite = sprites[sprite_index];
        map_file.Add(sprite);
      }
    }
    map_file.Write();
  }

  /**
   * Initializes the field component.
   * @param entity The field entity.
   */
  void cMap_Editor::Init_Field(tObject& entity) {
    entity["text"].Set_String("");
  }

  /**
   * Renders a field entity.
   * @param entity The field component.
   */
  void cMap_Editor::Render_Field(tObject& entity) {
    int dy = entity["height"].number - this->io->Get_Text_Height(entity["text"].string) / 2;
    int width = this->io->Get_Text_Width(entity["text"].string);
    int limit = entity["width"].number - 4;
    if (this->sel_component == entity["id"].string) { // Does field have input focus?
      if (width < limit) { // Only allow text if input has space.
        sSignal signal = this->io->Read_Key();
        if ((signal.code >= ' ') && (signal.code <= '~')) {
          entity["text"].string += (char)signal.code;
        }
        else if (signal.code == eSIGNAL_BACKSPACE) {
          entity["text"].string = entity["text"].string.substr(0, entity["text"].string.length() - 1); // Decrease string.
        }
        else if (signal.code = eSIGNAL_DELETE) {
          entity["text"].string = ""; // Clear out
        }
      }
      // Highlight the field.
      this->io->Box(0, 0, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h, 0, 255, 0);
    }
    else {
      this->io->Box(0, 0, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h, 0, 0, 0);
    }
    // Render the field.
    this->io->Box(1, 1, entity["width"].number * this->cell_w - 2, entity["height"].number * this->cell_h - 2, 255, 255, 255);
    this->io->Output_Text(entity["text"].string, 2, dy, 0, 0, 0);
  }

  /**
   * Initializes a grid view component.
   * @param entity The grid view entity.
   */
  void cMap_Editor::Init_Grid_View(tObject& entity) {
    Check_Condition(entity.Does_Key_Exist("columns"), "No column count specified for grid view.");
    entity["grid-x"].Set_Number(NO_VALUE_FOUND);
    entity["grid-y"].Set_Number(NO_VALUE_FOUND);
    entity["scroll-x"].Set_Number(0);
    entity["scroll-y"].Set_Number(0);
    entity["text"].Set_String("");
  }

  /**
   * Renders a grid view component.
   * @param entity The grid view entity.
   */
  void cMap_Editor::Render_Grid_View(tObject& entity) {
    cArray<std::string> data = Parse_Sausage_Text(entity["text"].string, ";");
    if ((data.Count() % entity["columns"].number) == 0) { // Does data match column count?
      int row_count = data.Count() / entity["columns"].number;
      int col_count = entity["columns"].number;
      int cell_width = entity["width"].number / col_count;
      int cell_height = this->io->Get_Text_Height(entity["text"].string) + 4;
      for (int grid_y = 0; grid_y < row_count; grid_y++) {
        for (int grid_x = 0; grid_x < col_count; grid_x++) {
          std::string& text = data[grid_y * col_count + grid_x];
          if (this->sel_component == entity["id"].string) { // Does field have input focus?
            sRectangle cell_map = { grid_x * cell_width - entity["scroll-x"].number,
                                    grid_y * cell_height - entity["scroll-y"].number,
                                    grid_x * cell_width + cell_width - 1 - entity["scroll-x"].number,
                                    grid_y * cell_height + cell_height - 1 - entity["scroll-y"].number };
            if (Is_Point_In_Box(this->mouse_coords, cell_map)) { // Check to see if we clicked into the cell.
              int width = this->io->Get_Text_Width(text);
              if (width < cell_width) { // Only allow text if input has space.
                sSignal signal = this->io->Read_Key();
                if ((signal.code >= ' ') && (signal.code <= '~')) {
                  text += (char)signal.code;
                }
                else if (signal.code == eSIGNAL_BACKSPACE) {
                  text = text.substr(0, text.length() - 1); // Decrease string.
                }
                else if (signal.code = eSIGNAL_DELETE) {
                  text = ""; // Clear out
                }
                this->Scroll_Component(entity, signal);
                entity["grid-x"].Set_Number(grid_x);
                entity["grid-y"].Set_Number(grid_y);
                entity["text"].Set_String(Join(data, ";")); // Update text.
              }
              // Highlight the field.
              this->io->Box(0 - entity["scroll-x"].number, 0 - entity["scroll-y"].number, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h, 0, 255, 0);
            }
          }
          else {
            this->io->Box(0 - entity["scroll-x"].number, 0 - entity["scroll-y"].number, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h, 0, 0, 0);
          }
          this->io->Box(grid_x * cell_width - entity["scroll-x"].number, grid_y * cell_height - entity["scroll-y"].number, cell_width, cell_height, 0, 0, 0);
          this->io->Box(grid_x * cell_width + 1 - entity["scroll-x"].number, grid_y * cell_height + 1 - entity["scroll-y"].number, cell_width - 2, cell_height - 2, 255, 255, 255);
          this->io->Output_Text(text, grid_x * cell_width + 2 - entity["scroll-x"].number, grid_y * cell_height + 2 - entity["scroll-y"].number, 0, 0, 0);
        }
      }
    }
  }

  /**
   * Renders the label component.
   * @param entity The label entity.
   */
  void cMap_Editor::Render_Label(tObject& entity) {
    Check_Condition(entity.Does_Key_Exist("label"), "No label specified for label.");
    Check_Condition(entity.Does_Key_Exist("red"), "Red component missing for label color.");
    Check_Condition(entity.Does_Key_Exist("green"), "Green component missing for label color.");
    Check_Condition(entity.Does_Key_Exist("blue"), "Blue component missing for label color.");
    this->io->Output_Text(entity["label"].string, 0, 0, entity["red"].number, entity["green"].number, entity["blue"].number);
  }

  /**
   * Initializes a list component.
   * @param entity The list entity.
   */
  void cMap_Editor::Init_List(tObject& entity) {
    entity["scroll-x"].Set_Number(0);
    entity["scroll-y"].Set_Number(0);
    entity["text"].Set_String("");
    entity["sel-item"].Set_Number(NO_VALUE_FOUND);
  }

  /**
   * Renders a list entity.
   * @param entity The entity component.
   */
  void cMap_Editor::Render_List(tObject& entity) {
    cArray<std::string> items = Parse_Sausage_Text(entity["text"].string, ";");
    int item_count = items.Count();
    int dy = entity["width"].number - this->io->Get_Text_Height(entity["text"].string) / 2;
    int height = this->io->Get_Text_Height(entity["text"].string) + 2;
    if (this->sel_component == entity["id"].string) { // Do we have input focus.
      sSignal signal = this->io->Read_Key();
      this->Scroll_Component(entity, signal);
    }
    for (int item_index = 0; item_index < item_count; item_index++) {
      std::string item = items[item_index];
      if (entity["sel-item"].number == item_index) {
        this->io->Box(0 - entity["scroll-x"].number, item_index * height - entity["scroll-y"].number, entity["width"].number, height, 0, 0, 128);
      }
      else {
        this->io->Box(0 - entity["scroll-x"].number, item_index * height - entity["scroll-y"].number, entity["width"].number, height, 255, 255, 255);
      }
      this->io->Output_Text(entity["text"].string, 2 - entity["scroll-x"].number, item_index * height + 2 - entity["scroll-y"].number, 0, 0, 0);
      if (this->clicked == entity["id"].string) { // Was item clicked?
        sRectangle item_map = { 0, item_index * height, entity["width"].number - 1, item_index * height + height - 1 };
        if (Is_Point_In_Box(this->mouse_coords, item_map)) { // Was item clicked on?
          entity["sel-item"].Set_Number(item_index);
          this->On_List_Click(entity, item);
        }
      }
    }
  }

  /**
   * Renders a button component.
   * @param entity The button entity.
   */
  void cMap_Editor::Render_Button(tObject& entity) {
    Check_Condition(entity.Does_Key_Exist("label"), "No label for button.");
    Check_Condition(entity.Does_Key_Exist("red"), "Missing red component for button color.");
    Check_Condition(entity.Does_Key_Exist("green"), "Missing green component for button color.");
    Check_Condition(entity.Does_Key_Exist("blue"), "Missing blue component for button color.");
    int dx = (entity["width"].number - this->io->Get_Text_Width(entity["label"].string)) / 2;
    int dy = (entity["height"].number - this->io->Get_Text_Height(entity["label"].string)) / 2;
    this->io->Box(0, 0, entity["width"].number * this->cell_w, entity["height"].number * this->cell_h, entity["red"].number, entity["green"].number, entity["blue"].number);
    this->io->Output_Text(entity["label"].string, dx, dy, entity["red"].number, entity["green"].number, entity["blue"].number);
    if (this->clicked == entity["id"].string) { // Was the button clicked?
      this->On_Button_Click(entity);
    }
  }

  /**
   * Called when the button was clicked.
   * @param entity The button entity.
   */
  void cMap_Editor::On_Button_Click(tObject& entity) {
    
  }

  /**
   * Initializes a toolbar component.
   * @param entity The toolbar entity.
   */
  void cMap_Editor::Init_Toolbar(tObject& entity) {
    Check_Condition(entity.Does_Key_Exist("columns"), "No column count specified for toolbar.");
    entity["scroll-x"].Set_Number(0);
    entity["scroll-y"].Set_Number(0);
    entity["text"].Set_String("");
    entity["item-x"].Set_Number(NO_VALUE_FOUND);
    entity["item-y"].Set_Number(NO_VALUE_FOUND);
  }

  /**
   * Renders the toolbar component.
   * @param entity The toolbar entity.
   */
  void cMap_Editor::Render_Toolbar(tObject& entity) {
    cArray<std::string> data = Parse_Sausage_Text(entity["text"].string, ";");
    if ((data.Count() % entity["columns"].number) == 0) {
      int row_count = data.Count() / entity["columns"].number;
      int col_count = entity["columns"].number;
      int cell_width = entity["width"].number / entity["columns"].number;
      if (this->sel_component == entity["id"].string) { // Do we have input focus.
        sSignal signal = this->io->Read_Key();
        this->Scroll_Component(entity, signal);
      }
      for (int grid_y = 0; grid_y < row_count; grid_y++) {
        for (int grid_x = 0; grid_x < col_count; grid_x++) {
          cArray<std::string> pair = Parse_Sausage_Text(data[grid_y * col_count + grid_x], ":");
          Check_Condition((pair.Count() == 2), "Invalid data format in toolbar item.");
          std::string label = pair[0];
          std::string icon = pair[1];
          int image_width = this->io->Get_Image_Width(icon);
          int image_height = this->io->Get_Image_Height(icon);
          int dx = (cell_width - image_width) / 2;
          int dy = (cell_width - image_height) / 2;
          int text_x = (cell_width - this->io->Get_Text_Width(label)) / 2;
          int text_y = dy + image_height + 1;
          if (this->clicked == entity["id"].string) {
            sRectangle box = { grid_x * cell_width - entity["scroll-x"].number,
                               grid_y * cell_width - entity["scroll-y"].number,
                               grid_x * cell_width + cell_width - 1 - entity["scroll-x"].number,
                               grid_y * cell_width + cell_width - 1 - entity["scroll-y"].number };
            if (Is_Point_In_Box(this->mouse_coords, box)) {
              entity["item-x"].Set_Number(grid_x);
              entity["item-y"].Set_Number(grid_y);
              this->On_Toolbar_Click(entity, label);
            }
          }
          this->io->Draw_Image(icon, dx - entity["scroll-x"].number, dy - entity["scroll-y"].number, image_width, image_height, 0, false, false);
          if ((entity["item-x"].number == grid_x) && (entity["item-y"].number == grid_y)) {
            this->io->Output_Text(label, text_x - entity["scroll-x"].number, text_y - entity["scroll-y"].number, 0, 128, 0); // Mark with green as this is selected.
          }
          else {
            this->io->Output_Text(label, text_x - entity["scroll-x"].number, text_y - entity["scroll-y"].number, 0, 0, 0);
          }
        }
      }
    }
  }

  /**
   * Initializes the map editor component.
   * @param entity The map editor entity.
   */
  void cMap_Editor::Init_Map_Editor(tObject& entity) {
    entity["scroll-x"].Set_Number(0);
    entity["scroll-y"].Set_Number(0);
  }

  /**
   * Renders the map editor component.
   * @param entity The map editor entity.
   */
  void cMap_Editor::Render_Map_Editor(tObject& entity) {
    
  }

  /**
   * Fires when a list item is clicked.
   * @param entity The list entity.
   * @param text The text of the item that was clicked.
   */
  void cMap_Editor::On_List_Click(tObject& entity, std::string text) {

  }

  /**
   * Fires when a toolbar item is clicked.
   * @param entity The toolbar entity.
   * @param label The label of the clicked item.
   */
  void cMap_Editor::On_Toolbar_Click(tObject& entity, std::string label) {

  }

  /**
   * Loads an object from a grid view.
   * @param object The object to load.
   * @param grid_view The grid view to load the object from.
   * @throws An error if there aren't two columns in the grid view.
   */
  void cMap_Editor::Load_Object_From_Grid_View(tObject& object, tObject& grid_view) {
    Check_Condition((grid_view["columns"].number == 2), "There needs to be two columns in grid view.");
    object.Clear();
    cArray<std::string> items = Parse_Sausage_Text(grid_view["text"].string, ";");
    int item_count = items.Count();
    Check_Condition((item_count % 2 == 0), "Data is not column aligned for object.");
    for (int item_index = 0; item_index < item_count; item_index += 2) {
      std::string& key = items[item_index + 0];
      cValue value = items[item_index + 1];
      object[key] = value;
    }
  }

  /**
   * Saves an object into a grid view.
   * @param object The object to save.
   * @param grid_view The grid view entity.
   * @throws An error if the grid view does not have two columns.
   */
  void cMap_Editor::Save_Object_To_Grid_View(tObject& object, tObject& grid_view) {
    Check_Condition((grid_view["columns"].number == 2), "There needs to be two columns in grid view.");
    this->Init_Grid_View(grid_view);
    int prop_count = object.Count();
    cArray<std::string> items;
    for (int prop_index = 0; prop_index < prop_count; prop_index++) {
      std::string& key = object.keys[prop_index];
      items.Add(key);
      cValue& value = object.values[prop_index];
      if (value.type == eVALUE_NUMBER) {
        value.Convert_To_String();
        items.Add(value.string);
      }
      else if (value.type == eVALUE_STRING) {
        items.Add(value.string);
      }
    }
    grid_view["text"].Set_String(Join(items, ";"));
  }

  /**
   * Updates the sprite palette.
   * @param toolbar The toolbar representing the sprite palette.
   */
  void cMap_Editor::Update_Sprite_Palette(tObject& toolbar) {
    this->Init_Toolbar(toolbar);
    int catalog_size = this->catalog.Count();
    cArray<std::string> items;
    for (int catalog_index = 0; catalog_index < catalog_size; catalog_index++) {
      std::string& sprite_name = this->catalog.keys[catalog_index];
      tObject& sprite = this->catalog.values[catalog_index];
      Check_Condition(sprite.Does_Key_Exist("icon"), "Icon property missing in sprite.");
      items.Add(sprite_name + ":" + sprite["icon"].string);
    }
    toolbar["text"].Set_String(Join(items, ";"));
  }

  /**
   * Scrolls a component when the user presses the arrow keys.
   * @param entity The component to scroll.
   * @param signal The user signal.
   */
  void cMap_Editor::Scroll_Component(tObject& entity, sSignal& signal) {
    Check_Condition(entity.Does_Key_Exist("scroll-x"), "Scroll x coordinate missing.");
    Check_Condition(entity.Does_Key_Exist("scroll-y"), "Scroll y coordinate missing.");
    switch (signal.code) {
      case eSIGNAL_LEFT: {
        entity["scroll-x"].number--;
        break;
      }
      case eSIGNAL_RIGHT: {
        entity["scroll-x"].number++;
        break;
      }
      case eSIGNAL_UP: {
        entity["scroll-y"].number--;
        break;
      }
      case eSIGNAL_DOWN: {
        entity["scroll-y"].number++;
      }
    }
  }

  /**
   * Updates the level list.
   * @param list The list component.
   */
  void cMap_Editor::Update_Levels(tObject& list) {
    cArray<std::string> files = this->io->Get_File_List(this->io->Get_Current_Folder());
    this->Init_List(list);
    cArray<std::string> levels;
    int file_count = files.Count();
    for (int file_index = 0; file_index < file_count; file_index++) {
      std::string file = files[file_index];
      levels.Add(this->io->Get_File_Title(file));
    }
    list["text"] = Join(levels, ";");
  }

  /**
   * Selects a sprite or created a new one.
   * @param signal The input signal.
   * @param map_editor The map editor component.
   */
  void cMap_Editor::Select_Sprite(sSignal& signal, tObject& map_editor) {
    tObject_List& sprites = this->sprite_layers[this->sel_layer];
    int sprite_count = sprites.Count();
    bool sprite_found = false;
    for (int sprite_index = sprite_count - 1; sprite_index >  0; sprite_index--) { // Select top-down sprites.
      tObject& sprite = sprites[sprite_index];
      Check_Condition(sprite.Does_Key_Exist("bump-map"), "No bump map present in sprite.");
      sRectangle bump_map = Parse_Rectangle(sprite["bump-map"].string);
      if (signal.code == eSIGNAL_MOUSE) {
        Check_Condition(sprite.Does_Key_Exist("x"), "Sprite has no X coordinate.");
        Check_Condition(sprite.Does_Key_Exist("y"), "Sprite has no Y coordinate.");
        // Augment bump map with sprite coordinates.
        bump_map.left += sprite["x"].number;
        bump_map.right += sprite["x"].number;
        bump_map.top += sprite["y"].number;
        bump_map.bottom += sprite["y"].number;
        if (Is_Point_In_Box(this->mouse_coords, bump_map)) {
          if (signal.button == eBUTTON_LEFT) {
            if (map_editor["sel-sprite"].number == NO_VALUE_FOUND) { // Sprite not selected.
              map_editor["sel-sprite"].Set_Number(sprite_index);
              sprite_found = true;
              break;
            }
          }
        }
      }
    }
    if (!sprite_found) { // Lay down sprite if no sprite found.
      if (map_editor["sel-sprite"].number == NO_VALUE_FOUND) {
        tObject& new_sprite = this->catalog[this->sel_sprite_id];
        sprites.Add(new_sprite);
        map_editor["sel-sprite"].Set_Number(sprites.Count() - 1);
      }
    }
  }

  /**
   * Parses a rectangle from text.
   * @param text The text containing the rectangle string.
   * @return A rectangle object.
   * @throws An error if the rectangle is not valid.
   */
  sRectangle cMap_Editor::Parse_Rectangle(std::string text) {
    cArray<std::string> str_rect = Parse_Sausage_Text(text, ",");
    sRectangle rect;
    Check_Condition((str_rect.Count() == 4), "Rectangle is not formatted correctly.");
    rect.left = Text_To_Number(str_rect[0]);
    rect.top = Text_To_Number(str_rect[1]);
    rect.right = Text_To_Number(str_rect[2]);
    rect.bottom = Text_To_Number(str_rect[3]);
    return rect;
  }

  /**
   * Renders sprites to the map control.
   * @param map_editor The map editor component.
   */
  void cMap_Editor::Render_Sprites(tObject& map_editor) {
    int bkg_width = this->io->Get_Image_Width(this->meta_data["background"].string);
    int bkg_height = this->io->Get_Image_Height(this->meta_data["background"].string);
    int map_width = map_editor["width"].number * this->cell_w;
    int map_height = map_editor["height"].number * this->cell_h;
    Check_Condition((bkg_width == map_width) && (bkg_height == map_height), "The size of the background must match the map size. (" + Number_To_Text(map_width) + "x" + Number_To_Text(map_height) + ")");
    this->io->Draw_Image(this->meta_data["background"].string, 0, 0, map_width, map_height, 0, false, false);
    int layer_count = this->sprite_layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      tObject_List& layer = this->sprite_layers.values[layer_index];
      int sprite_count = layer.Count();
      for (int sprite_index = 0; sprite_index < sprite_count; sprite_index++) {
        tObject& sprite = layer[sprite_index];
        Check_Condition(sprite.Does_Key_Exist("x"), "No X coordinate in sprite.");
        Check_Condition(sprite.Does_Key_Exist("y"), "No Y coordinate in sprite.");
        Check_Condition(sprite.Does_Key_Exist("width"), "Sprite has no width set.");
        Check_Condition(sprite.Does_Key_Exist("height"), "Sprite has no height set.");
        this->io->Draw_Image(sprite["icon"].string, sprite["x"].number - map_editor["scroll-x"].number, sprite["y"].number - map_editor["scroll-y"].number, sprite["width"].number, sprite["height"].number, 0, false, false);
      }
    }
  }

  /**
   * Clears out the map data.
   */
  void cMap_Editor::Clear_Map() {
    this->sel_layer = "background";
    this->sel_sprite = NO_VALUE_FOUND;
    this->meta_data.Clear();
    int layer_count = this->sprite_layers.Count();
    for (int layer_index = 0; layer_index < layer_count; layer_index++) {
      tObject_List& layer = this->sprite_layers.values[layer_index];
      layer.Clear();
    }
  }

  /**
   * Destars starred sprite properties.
   * @param sprite The sprite properties to destar.
   */
  void cMap_Editor::Destar_Sprite(tObject& sprite) {
    int prop_count = sprite.Count();
    for (int prop_index = 0; prop_index < prop_count; prop_index++) {
      Check_Condition((sprite.keys[prop_index].length() > 0), "Key is NULL.");
      if (sprite.keys[prop_index][0] == '*') {
        sprite.keys[prop_index] = sprite.keys[prop_index].substr(1); // Destar the key.
      }
    }
  }

}