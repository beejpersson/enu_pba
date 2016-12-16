#include "game.h"
#include "physics.h"
#include "cPhysicsComponents.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>

using namespace std;
using namespace graphics_framework;
using namespace glm;
#define physics_tick 1.0 / 60.0

static vector<unique_ptr<Entity>> SceneList;
static unique_ptr<Entity> floorEnt;

//Initialising variables
double cursor_x = 0.0;
double cursor_y = 0.0;
int cam_select = 0;
free_camera free_cam;
float cam_speed;
target_camera targ_cam;
target_camera rot_cam;
int scene_select = 0;

bool initialise()
{
	// Set input mode - hide the cursor
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Capture initial mouse position
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);

	return true;
}

// Sphere creator
unique_ptr<Entity> CreateParticle(const vec3 &position)
{
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	unique_ptr<Component> physComponent(new cParticle());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}

// Box creator
unique_ptr<Entity> CreateBox(const vec3 &position)
{
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(position);
	ent->SetRotation(angleAxis(-45.0f, vec3(1, 0, 0)));
	unique_ptr<Component> physComponent(new cRigidCube());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::BOX));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->SetName("Cuby");
	ent->AddComponent(unique_ptr<Component>(new cBoxCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));

	return ent;
}

bool update(float delta_time)
{
	// Timing update using fixed timestep
	static double t = 0.0;
	static double accumulator = 0.0;
	accumulator += delta_time;
	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	// Press space to rotate cubes
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_SPACE)) {
		for (auto &e : SceneList) {
			auto b = e->getComponent<cRigidCube>();
			if (b != NULL) {
				b->AddAngularForce({ 0, 0, 5.0 });
			}
		}
	}

	// Press arrow keys to move ball
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
	{
		SceneList.at(0)->getComponent<cParticle>()->AddLinearForce({ 0.0, 0.0, -20.0 });
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
	{
		SceneList.at(0)->getComponent<cParticle>()->AddLinearForce({ 0.0, 0.0, 20.0 });
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT))
	{
		SceneList.at(0)->getComponent<cParticle>()->AddLinearForce({ -20.0, 0.0, 0.0 });
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_RIGHT))
	{
		SceneList.at(0)->getComponent<cParticle>()->AddLinearForce({ 20.0, 0.0, 0.0 });
	}

	// Update objects in scene list
	for (auto &e : SceneList) {
		e->Update(delta_time);
	}

	// Use keyboard numbers to change camera
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_5))
		scene_select = 0;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_6))
		scene_select = 1;

	// The ratio of pixels to rotation
	static double ratio_width = quarter_pi<float>() / static_cast<float>(renderer::get_screen_width());
	static double ratio_height = (quarter_pi<float>() * (static_cast<float>(renderer::get_screen_height()) / static_cast<float>(renderer::get_screen_width()))) / static_cast<float>(renderer::get_screen_height());
	// Create doubles to store new cursor position
	double current_x;
	double current_y;
	// Get the current cursor position
	glfwGetCursorPos(renderer::get_window(), &current_x, &current_y);
	// Calculate delta of cursor positions from last frame
	double delta_x = current_x - cursor_x;
	double delta_y = current_y - cursor_y;
	// Multiply deltas by ratios - gets actual change in orientation
	delta_x *= ratio_width;
	delta_y *= ratio_height;
	// Rotate cameras by delta
	free_cam.rotate(float(delta_x), float(-delta_y));

	// Use WASD on keyboard to move the camera and shift to move faster
	if (cam_select == 0)
	{
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT_SHIFT))
			cam_speed = 50.0f;
		else cam_speed = 10.0f;
		if (glfwGetKey(renderer::get_window(), 'W'))
			free_cam.move(vec3(0.0f, 0.0f, 1.0f) * cam_speed * delta_time);
		if (glfwGetKey(renderer::get_window(), 'S'))
			free_cam.move(vec3(0.0f, 0.0f, -1.0f) * cam_speed * delta_time);
		if (glfwGetKey(renderer::get_window(), 'A'))
			free_cam.move(vec3(-1.0f, 0.0f, 0.0f) * cam_speed * delta_time);
		if (glfwGetKey(renderer::get_window(), 'D'))
			free_cam.move(vec3(1.0f, 0.0f, 0.0f) * cam_speed * delta_time);
	}

	// Update the free camera
	free_cam.update(delta_time);

	// Update cursor pos
	cursor_x = current_x;
	cursor_y = current_y;

	// Update the target camera
	targ_cam.update(delta_time);

	// Use keyboard numbers to change camera
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_1))
		cam_select = 0;
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_2))
	{
		targ_cam.set_position(vec3(20.0f, 10.0f, 20.0f));
		cam_select = 1;
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_3))
	{
		targ_cam.set_position(vec3(20.0f, 10.0f, 20.0f));
		cam_select = 2;
	}

	// Set camera position and target depending on chosen camera
	if (cam_select == 0)
	{
		phys::SetCameraPos(free_cam.get_position());
		phys::SetCameraTarget(free_cam.get_target());
	}
	else if (cam_select == 1)
	{
		phys::SetCameraPos(targ_cam.get_position());
		phys::SetCameraTarget(targ_cam.get_target());
	}
	else if (cam_select == 2)
	{
		static float rot = 0.0f;
		rot += 0.2f * delta_time;
		phys::SetCameraPos(rotate(vec3(15.0f, 12.0f, 15.0f), rot, vec3(0, 1.0f, 0)));
		phys::SetCameraTarget(targ_cam.get_target());
	}

	phys::Update(delta_time);
	return true;
}

bool load_content()
{
	// Load scene list content
	phys::Init();
	SceneList.push_back(move(CreateParticle({ 20.0, 0.0, 0.0 })));
	for (size_t i = 0; i < 4; i++)
	{
		SceneList.push_back(move(CreateParticle({ -2.0, 5.0 + (double)(rand() % 200) / 20.0, 0.0 })));
	}

	for (size_t i = 0; i < 4; i++)
	{
		SceneList.push_back(move(CreateParticle({ -2.0, 5.0 + (double)(rand() % 200) / 20.0, 8.0 })));
	}

	for (size_t i = 0; i < 5; i++)
	{
		SceneList.push_back(move(CreateBox({ 0, 2*i, 2*i })));
	}


	// Load floor
	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));
	floorEnt->SetName("Floor");

	// Set free_camera properties
	free_cam.set_position(vec3(0.0f, 10.0f, 0.0f));
	free_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	auto aspect = static_cast<float>(renderer::get_screen_width()) / static_cast<float>(renderer::get_screen_height());
	free_cam.set_projection(quarter_pi<float>(), aspect, 0.1f, 1000.0f);

	// Set target_camera properties
	targ_cam.set_position(vec3(20.0f, 10.0f, 20.0f));
	targ_cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	aspect = static_cast<float>(renderer::get_screen_width()) / static_cast<float>(renderer::get_screen_height());
	targ_cam.set_projection(quarter_pi<float>(), aspect, 0.1f, 1000.0f);

	InitPhysics();
	return true;
}

bool render() {

	// Render desired scene
	if (scene_select == 0)
	{
		phys::DrawSphere(glm::vec3(4.0f, 4.0f, 0), 1.0f, RED);
		phys::DrawSphere(glm::vec3(-4.0f, 4.0f, 0), 1.0f, RED);
		phys::DrawSphere(glm::vec3(0, 8.0f, 0), 0.2f, YELLOW);
		phys::DrawSphere(glm::vec3(0), 1.0f, GREEN);
		phys::DrawSphere(glm::vec3(0, 4.0f, 4.0f), 1.0f, BLUE);
		phys::DrawSphere(glm::vec3(0, 4.0f, -4.0f), 1.0f, BLUE);
		phys::DrawCube(glm::vec3(0, 4.0f, 0));
		phys::DrawLine(glm::vec3(0, 4.0f, 4.0f), glm::vec3(0, 8.0f, 0));
		phys::DrawLine(glm::vec3(0, 4.0f, -4.0f), glm::vec3(0, 8.0f, 0));
		phys::DrawLine(glm::vec3(4.0f, 4.0f, 0), glm::vec3(0), true, ORANGE);
		phys::DrawLine(glm::vec3(-4.0f, 4.0f, 0), glm::vec3(0), true, PINK);
		phys::DrawLineCross(glm::vec3(0, 8.0f, 0), 1.0f, false);
		phys::DrawArrow(glm::vec3(0, 4.0f, 0), glm::vec3(0, 8.0f, 0), 1.0f, GREY);
	}
	else if (scene_select == 1)
	{
		for (auto &e : SceneList) {
			e->Render();
		}
	}

	phys::DrawScene();
	return true;
}

void main() {
	// Create application
	app application;
	// Set load content, update and render methods
	application.set_initialise(initialise);
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}