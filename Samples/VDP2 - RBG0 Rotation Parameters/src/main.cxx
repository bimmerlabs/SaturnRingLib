/*VDP2 Scroll Layer Demo:
This Demo Shows how to Load and display 2 Tilemaps on RBG0 simultaneously using separate
Rotation Parameters
*/
#include <srl.hpp>


using namespace SRL::Types;
using namespace SRL::Input;
using namespace SRL::Math;



int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
    Digital port0(0); // Initialize gamepad on port 0
    
    // variables to store current position/ rotation of the RBG0 tilemaps:
    Vector3D Rbg0PositionPrimary(0.0,40.0,80.0);//40 units below camera for ground
    Vector3D Rbg0PositionSecondary(0.0,-40.0,80.0);//40 units below camera for ground
    Angle Rbg0RotationSecondary = Angle::FromDegrees(0.0);//we will also make the clouds rotate
    
    SRL::Tilemap::Interfaces::CubeTile* TestTilebin= new SRL::Tilemap::Interfaces::CubeTile("GROUND.BIN");//Load ground tilemap from cd to work RAM
    
    SRL::Tilemap::Interfaces::CubeTile *TestTilebinB= new SRL::Tilemap::Interfaces::CubeTile("CLOUDS.BIN");//Load fog tilemap from cd to work RAM
    
    //This new overload allows transferring both tilemaps to VRAM and initializing their rotation modes all in one go:
    SRL::VDP2::RBG0::LoadTilemap(*TestTilebin,*TestTilebinB, SRL::VDP2::RotationMode::TwoAxis,SRL::VDP2::RotationMode::TwoAxis);
    
    delete TestTilebin;//free work RAM 
    delete TestTilebinB;//free work RAM
    
    //Example of laying out a multi plane tilemap in a 4x4 grid for the Primary Tilemap 
    uint8_t mapLayout[4][4] = 
    {
        0,1,2,3,
        0,1,2,3,
        0,1,2,3,
        0,1,2,3,
    };

    //set the multi plane layout of Primary Tilemap 
    SRL::VDP2::RBG0::SetPlanes(mapLayout);
    
    //set initial perspective for the Primary and secondary rotation:
    SRL::Scene3D::LoadIdentity();
    
    SRL::Scene3D::PushMatrix();
    {
        SRL::Scene3D::Translate(Rbg0PositionPrimary);
        SRL::Scene3D::RotateX(Angle::FromDegrees(-90.0));
        SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Primary);
    }

    SRL::Scene3D::PushMatrix();
    {
        SRL::Scene3D::Translate(Rbg0PositionSecondary);
        SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));
        SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Secondary);
    }
    SRL::Scene3D::PopMatrix();
    
    /*This is not strictly necessary in our example because PerspectiveSwitch is the default setting
    when loading 2 tilemaps, but use this function if you want a different setting*/
    SRL::VDP2::RBG0::SetParameterDisplay(SRL::VDP2::SwitchMode::PerspectiveSwitch);
    
    //Enable RBG0 to display on screen:
    SRL::VDP2::RBG0::ScrollEnable(); 
    
    SRL::Debug::Print(1,3,"VDP2 Rotation Parameters Sample");
   
    while(1)
    {    
        //Demonstrate independent manipulation of RA and RB tilemaps: 
        Rbg0PositionPrimary.Z-=Fxp(1.0);//scroll ground towards camera
        Rbg0RotationSecondary+=Angle::FromDegrees(1.0);//rotate clouds
       
       
        SRL::Scene3D::LoadIdentity();
        //set perspective matrix of primary tilemap:
        SRL::Scene3D::PushMatrix();
        {
            SRL::Scene3D::Translate(Rbg0PositionPrimary);
            SRL::Scene3D::RotateX(Angle::FromDegrees(-90.0));
            SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Primary);
        }
        SRL::Scene3D::PopMatrix();

        //set perspective matrix of secondary tilemap:
        SRL::Scene3D::PushMatrix();
        {
            SRL::Scene3D::Translate(Rbg0PositionSecondary);
            SRL::Scene3D::RotateY(Rbg0RotationSecondary);
            SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));
            SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Secondary);
        }
        SRL::Scene3D::PopMatrix();
    
        SRL::Core::Synchronize();
    }
    return 0;
}
