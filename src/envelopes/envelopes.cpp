#include "envelopes/envelopes.h"
#include "envelopes/borolope.h"

stage_t operator++ (stage_t& d) {
    d = static_cast<stage_t>((static_cast<int>(d) + 1) % 6);
    return d;
}

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "menuitems_object.h"
    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_envelopegraph.h"

    FLASHMEM
    void EnvelopeBase::make_menu_items(Menu *menu, int index) {
        char label[40];
        snprintf(label, 40, "Envelope %i: %s", index, this->label);
        menu->add_page(label, C_WHITE, false);

        menu->add(new EnvelopeDisplay("Graph", this));
        menu->add(new EnvelopeIndicator("Indicator", this));
    }

    FLASHMEM
    void RegularEnvelope::make_menu_items(Menu *menu, int index) {
        EnvelopeBase::make_menu_items(menu, index);
        //#ifdef ENABLE_ENVELOPE_MENUS
            //menu->add(new EnvelopeDisplay("Graph", this));
            //menu->add(new EnvelopeIndicator("Indicator", this));

            SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 5);
            sub_menu_item_columns->show_header = false;

            // todo: convert all these ObjectNumberControls and ObjectToggleControls into LambdaNumberControls and LambdaToggleControls
            sub_menu_item_columns->add(new ObjectNumberControl<RegularEnvelope,float>("Att", this, &RegularEnvelope::set_attack,  &RegularEnvelope::get_attack,    nullptr, 0, 1.0, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<RegularEnvelope,float>("Hld", this, &RegularEnvelope::set_hold,    &RegularEnvelope::get_hold,      nullptr, 0, 1.0, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<RegularEnvelope,float>("Dec", this, &RegularEnvelope::set_decay,   &RegularEnvelope::get_decay,     nullptr, 0, 1.0, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<RegularEnvelope,float>("Sus", this, &RegularEnvelope::set_sustain, &RegularEnvelope::get_sustain,   nullptr, 0, 1.0, true, true));
            sub_menu_item_columns->add(new ObjectNumberControl<RegularEnvelope,float>("Rel", this, &RegularEnvelope::set_release, &RegularEnvelope::get_release,   nullptr, 0, 1.0, true, true));

            menu->add(sub_menu_item_columns);

            SubMenuItemBar *typebar = new SubMenuItemBar("Type");
            typebar->show_header = false;
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Inverted",  this, &EnvelopeBase::set_invert,  &EnvelopeBase::is_invert,     nullptr));
            typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Looping",   this, &EnvelopeBase::set_loop,    &EnvelopeBase::is_loop,       nullptr));

            //SubMenuItemBar *mod = new SubMenuItemBar("Modulation");
            typebar->add(new ObjectNumberControl<RegularEnvelope,int8_t>("Mod HD",  this, &RegularEnvelope::set_mod_hd,  &RegularEnvelope::get_mod_hd,    nullptr, 0, 127, true, true));
            typebar->add(new ObjectNumberControl<RegularEnvelope,int8_t>("Mod SR",  this, &RegularEnvelope::set_mod_sr,  &RegularEnvelope::get_mod_sr,    nullptr, 0, 127, true, true));

            typebar->add(new ObjectNumberControl<RegularEnvelope,uint8_t>("Sync", this, &RegularEnvelope::set_cc_value_sync_modifier, &RegularEnvelope::get_cc_value_sync_modifier, nullptr, 1, 24, true, true));

            menu->add(typebar);
            //menu->add(mod);
        //#endif
    }

    FLASHMEM
    void Weirdolope::make_menu_items(Menu *menu, int index) {
        EnvelopeBase::make_menu_items(menu, index);

        /*SubMenuItemBar *sub_menu_item_columns = new SubMenuItemBar("Options");
        sub_menu_item_columns->show_header = false;
        sub_menu_item_columns->add(new ObjectNumberControl<Weirdolope,float>("Mix", this, &Weirdolope::setMix, &Weirdolope::getMix, nullptr, 0.0f, 1.0f, true, true));
        menu->add(sub_menu_item_columns);*/

        SubMenuItemBar *typebar = new SubMenuItemBar("Type");
        typebar->show_header = false;
        // todo: convert these to Lambdas
        typebar->add(new ObjectNumberControl<Weirdolope,float> ("Mix",       this, &Weirdolope::setMix,        &Weirdolope::getMix,          nullptr, 0.0f, 10.0f, true, true));
        typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Inverted",  this, &EnvelopeBase::set_invert,  &EnvelopeBase::is_invert,     nullptr));
        typebar->add(new ObjectToggleControl<EnvelopeBase>     ("Looping",   this, &EnvelopeBase::set_loop,    &EnvelopeBase::is_loop,       nullptr));

        menu->add(typebar);
    }
#endif

#ifdef ENABLE_PARAMETERS
    #include "parameters/Parameter.h"

    LinkedList<FloatParameter*> *RegularEnvelope::get_parameters() {
        if (this->parameters!=nullptr)
            return this->parameters;

        this->parameters = new LinkedList<FloatParameter*>();
        
        this->parameters->add(new LDataParameter<float>("Attack", [=](float v) -> void { this->set_attack(v); }, [=](void) -> float { return this->get_attack(); }, 0, 127));
        this->parameters->add(new LDataParameter<float>("Hold",   [=](float v) -> void { this->set_hold(v); },   [=](void) -> float { return this->get_hold();   }, 0, 127));
        this->parameters->add(new LDataParameter<float>("Decay",  [=](float v) -> void { this->set_decay(v); },  [=](void) -> float { return this->get_decay();  }, 0, 127));
        this->parameters->add(new LDataParameter<float>("Sustain",[=](float v) -> void { this->set_sustain(v); },[=](void) -> float { return this->get_sustain(); },0, 127));
        this->parameters->add(new LDataParameter<float>("Release",[=](float v) -> void { this->set_release(v); },[=](void) -> float { return this->get_release(); },0, 127));

        return this->parameters;
    }

    LinkedList<FloatParameter*> *Weirdolope::get_parameters() {
        if (this->parameters!=nullptr)
            return this->parameters;

        this->parameters = new LinkedList<FloatParameter*>();
        
        this->parameters->add(new LDataParameter<float>(
            "Mix",
            [=](float v) -> void { this->setMix(v); },
            [=](void) -> float { return this->getMix(); },
            0.0f,
            10.0f
        ));
        //this->parameters->back()->debug = true;

        return this->parameters;
    }
#endif