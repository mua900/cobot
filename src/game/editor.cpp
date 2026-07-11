#include "editor.hpp"
#include "log.hpp"

void VehicleEditor::set_part_position(cobot::vec2 where, AttachmentDistance dist, VPartData& partData, bool firstRoot)
{
    if (dist.point)
    {
        partData.parent = dist.parent;
    }

    if (rootPart)
    {
        partData.transform.position = firstRoot ? cobot::vec2() : where - vehicle.worldPosition;
    }
}

bool VehicleEditor::check_placement(AttachmentDistance dist, const char** errorMessage)
{
    if (dist.point && rootPart)
    {
        // trying to attach a root part to an attachment point
        *errorMessage = "Root parts can not be attached to other parts";
        return false;
    }
    if (!(dist.point || rootPart))
    {
        // trying to attach a part that isn't root and also isn't attached to anything
        *errorMessage = "Non-root parts must attach to something or they are not a part of the vehicle";
        return false;
    }

    return true;
}

bool VehicleEditor::place_part(cobot::vec2 where, const char** errorMessage)
{
    if (!haveSelectedPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    if (!check_placement(dist, errorMessage))
    {
        vehicle.remove_part(selectedPart);

#if EDITOR_DEBUG
        log_debug("Removed part: %u", selectedPart);
#endif

        return false;
    }

    bool firstRootPart = rootPart && (vehicle.rootParts.count() == 0);
    VehiclePart& part = vehicle.get_part(selectedPart);

    set_part_position(where, dist, part.partData, firstRootPart);

    if (dist.point)
    {
        dist.point->attach(selectedPart);
    }

    if (firstRootPart)
    {
        vehicle.worldPosition = where;
    }
    
    if (rootPart)
    {
        vehicle.add_root(selectedPart);
    }

#if EDITOR_DEBUG
    log_debug("Attached part: %u", selectedPart);
#endif

    return true;
}

bool VehicleEditor::place_editor_part(cobot::vec2 where, const char** errorMessage)
{
    if (!haveSelectedEditorPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    PartKind kind = selectedEditorPartKind;

    if (!check_placement(dist, errorMessage))
    {
        return false;
    }

    bool firstRootPart = rootPart && (vehicle.rootParts.count() == 0);
    VehiclePart part (kind);
    part.init();

    set_part_position(where, dist, part.partData, firstRootPart);

    PartId id = vehicle.add_part(part);

    if (dist.point)
    {
        dist.point->attach(id);
    }
    
    if (firstRootPart)
    {
        vehicle.worldPosition = where;
    }

    if (rootPart)
    {
        vehicle.add_root(id);
    }

#if EDITOR_DEBUG
    log_debug("Attached part kind: %u", selectedEditorPartKind);
#endif

    return true;
}

void VehicleEditor::draw_selected_part(SDL_Texture* part, RenderContext& render, cobot::vec2 where)
{
    cobot::vec2 textureSize;
    SDL_GetTextureSize(part, &textureSize.x, &textureSize.y);

    cobot::RectangleRot area = {};
    if (haveSnap)
    {
        area = cobot::RectangleRot(snap.position, textureSize, snap.rotation);
    }
    else
    {
        area = cobot::RectangleRot(where, textureSize, 0);
    }

    cobot::Quad points = area.get_points();
    draw_quad_with_texture(render, points, part, cobot::ColorF(0.9,0.9,0.9,0.5));
}


bool input_mouse_vehicle_editor(VehicleEditor& editor, Input& input, UiState& ui, const Camera* camera)
{
    cobot::vec2 ms = input.mouse.pos;
    cobot::vec2 mouseWorld = camera->screen_to_world(ms);

    PartId part = editor.vehicle.getPartAt(mouseWorld);
    if (part != NullPartId)
    {
        if (input.mouse.buttonFlags & MOUSE_LEFT_MASK)
        {
            editor.vehicle.unattach_from_parent(part);

            editor.selectedPart = part;
            editor.haveSelectedPart = true;
            return true;
        }
        else if (input.mouse.buttonFlags & MOUSE_RIGHT_MASK)
        {
            ValuePanel* properties = ui.get_value_panel(PartPropertyPanel);
            ASSERT(properties);

            // @todo set values

            return true;
        }
    }

    return false;
}

bool input_keyboard_vehicle_editor(VehicleEditor& editor, Input& input)
{
    return false;
}

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages)
{
	const Camera* camera = render.camera;
	ASSERT(camera);

    cobot::vec2 ws = render.render_size;

    auto mouseWorld = camera->screen_to_world(input.mouse.pos);

	draw_vehicle_editor_background(render, catalog, input, editor);
	
    if (editor.haveSelectedEditorPart)
    {
        SDL_Texture* texture = get_part_texture(editor.selectedEditorPartKind, catalog);
        editor.draw_selected_part(texture, render, mouseWorld);
    }
    else if (editor.haveSelectedPart)
    {
        SDL_Texture* texture = get_part_texture(editor.vehicle.get_part(editor.selectedPart).kind, catalog);
        editor.draw_selected_part(texture, render, mouseWorld);
    }

    draw_vehicle(render, catalog, editor.vehicle, VehicleDrawParameters(&partImages, NullPartId, true));
}

void draw_vehicle_editor_background(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor)
{
	draw_arc(render, cobot::vec2(0,0), 300, 320, 10 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
	draw_arc(render, cobot::vec2(0,0), 300, 320, 190 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
}

bool initialize_vehicle_editor_ui(cobot::vec2 windowSize, AssetId fontId, UiState& ui, AssetCatalog& catalog, RenderContext& render)
{
    Font font = catalog.get_font(fontId);

    float panelTitleHeight = 10;
    cobot::Rectangle panel_area = { 0, panelTitleHeight, windowSize.x * 0.3f, windowSize.y * 0.9f };
    cobot::Color panel_color = cobot::Color(0x33, 0x44, 0x44);
    Panel partsPanel (PartsPanel, panel_area.to_center(), 32, 48, 16);
    partsPanel.title_height = panelTitleHeight;
    partsPanel.title_bar_color = cobot::Color(0x44, 0x66, 0x77);

    cobot::Color iconColor = cobot::Color(0x77, 0x33, 0x44);
    cobot::Color tabIconColor = cobot::Color(0x33, 0x66, 0x44);

    // @todo we can delegate categorization to to load_part_icons which would be better
    DArray<IconButton> partIcons;
    if (!load_part_icons(partIcons, iconColor, catalog))
    {
        return false;
    }

    DArray <IconButton> groundTabIcons;
    DArray<IconButton> structureTabIcons;
    DArray<IconButton> computerTabIcons;
    DArray<IconButton> powerTabIcons;
    DArray<IconButton> instrumentTabIcons;

    for (auto& icon : partIcons)
    {
        PartKind kind = PartKind(icon.data.number);
        switch (getPartCategory(kind))
        {
            case CategoryPower:         powerTabIcons.add(icon);      break;
            case CategoryComputer:      computerTabIcons.add(icon);   break;
            case CategoryStructure:     structureTabIcons.add(icon);  break;
            case CategoryGround:        groundTabIcons.add(icon);     break;
            case CategoryInstrument:    instrumentTabIcons.add(icon); break;
            default:
                panic("Invalid part category");
        }
    }

    partIcons.reset();

    AssetId groundIconId = get_asset(String("groundTabIcon"), catalog);
    if (!groundIconId.is_valid()) return false;
    Icon groundIcon = Icon(catalog.get_image(groundIconId), tabIconColor);

    AssetId structureIconId = get_asset(String("structuralTabIcon"), catalog);
    if (!structureIconId.is_valid()) return false;
    Icon structureIcon = Icon(catalog.get_image(structureIconId), tabIconColor);
    
    AssetId computerIconId = get_asset(String("computerTabIcon"), catalog);
    if (!computerIconId.is_valid()) return false;
    Icon computerIcon = Icon(catalog.get_image(computerIconId), tabIconColor);
    
    AssetId powerIconId = get_asset(String("powerTabIcon"), catalog);
    if (!powerIconId.is_valid()) return false;
    Icon powerIcon = Icon(catalog.get_image(powerIconId), tabIconColor);

    AssetId instrumentIconId = get_asset(String("instrumentTabIcon"), catalog);
    if (!instrumentIconId.is_valid()) return false;
    Icon instrumentIcon = Icon(catalog.get_image(instrumentIconId), tabIconColor);

    partsPanel.tabs.add(PanelTab(groundIcon, groundTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(structureIcon, structureTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(computerIcon, computerTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(powerIcon, powerTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(instrumentIcon, instrumentTabIcons, panel_color));

    cobot::Rectangle propertiesPanelArea = {
        windowSize.x * 0.9f, windowSize.y * 0.5f,
        windowSize.x * 0.2f, windowSize.y * 0.5f
    };
    cobot::ColorF propertiesPanelColor = {
        0.1f, 0.5f, 0.2f
    };

    ValuePanel propertiesPanel = ValuePanel(PartPropertyPanel, propertiesPanelArea, 25, 32, cobot::DirWest);

    ValuePanelTab properties[PartKindCount];
    for (int i = 0; i < PartKindCount; i++)
    {
        ValuePanelTab& prop = properties[i];
        prop.tabIcon = Icon(nullptr, cobot::Color(0x55, 0x66, 0x44));
        prop.color = propertiesPanelColor;
        prop.field_height = 32;
        prop.field_margin = 8;
        propertiesPanel.tabs.add(prop);
    }

    cobot::Color background = cobot::Color(0x44, 0x88, 0x55);
    cobot::Color textColor = cobot::Color(0xAA, 0xAA, 0x66);
    TextButton backButton = TextButton(create_text(render.renderer, String("Back"), font, textColor), cobot::vec2(windowSize.x * 0.95, windowSize.y * 0.05), windowSize * 0.1, background);
    backButton.id = BackButton;
    TextButton saveVehicle = TextButton(create_text(render.renderer, String("Save"), font, textColor), windowSize * 0.95, windowSize * 0.1, background);
    saveVehicle.id = SaveVehicle;

    float nameFieldHeight = 100;
    float nameFieldWidth = 400;

    ui.panel.add(partsPanel);
    ui.value_panel.add(propertiesPanel);
    ui.button.add(saveVehicle);
    ui.button.add(backButton);
    ui.text_field.add(Text_Field(cobot::Rectangle(windowSize.x * 0.5, nameFieldHeight / 2, nameFieldWidth, nameFieldHeight), fontId, cobot::Color(0x55, 0x33, 0x44), cobot::Color(0x99, 0xAA, 0xBB), VehicleName));

    return true;
}