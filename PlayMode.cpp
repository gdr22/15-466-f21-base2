#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <time.h>

GLuint catblob_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > catblob_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("CatMesh.pnct"));
	catblob_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > catblob_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("Cat.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = catblob_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = catblob_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::Block PlayMode::new_block(float angle, float depth) {
	Block block;

	block.angle = angle;
	block.depth = depth;
	block.tile = new Scene::Transform;

	block.tile->position = grass->position;
	block.tile->rotation = grass->rotation;
	block.tile->scale    = grass->scale;

	block.tile->name = "Grass Copy";
	//scene.transforms.push_back(tile);

	Scene::Drawable drawable(block.tile);
	drawable.pipeline = lit_color_texture_program_pipeline;
	drawable.pipeline.vao = catblob_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = grass_vertex_type;
	drawable.pipeline.start = grass_vertex_start;
	drawable.pipeline.count = grass_vertex_count;
	PlayMode::scene.drawables.push_back(drawable);

	
	block.update_pos(rotation);
	blocks.emplace_back(block);
	return block;
}

PlayMode::PlayMode() : scene(*catblob_scene) {
	//init game states
	score = 0;

	//get pointers:
	for (auto& drawable : scene.drawables) {
		//sadness that switch doesnt work on string
		if (drawable.transform->name == "Grass") {
			grass = drawable.transform;
			grass_vertex_type = drawable.pipeline.type;
			grass_vertex_start = drawable.pipeline.start;
			grass_vertex_count = drawable.pipeline.count;
		} 
		else if (drawable.transform->name == "Left Back Foot") {
			lbpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Left Ear") {
			lear = drawable.transform;
		}
		else if (drawable.transform->name == "Left Front Foot") {
			lfpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Right Back Foot") {
			rbpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Right Ear") {
			rear = drawable.transform;
		}
		else if (drawable.transform->name == "Right Front Foot") {
			rfpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Sphere") {
			cat = drawable.transform;
		}
		else if (drawable.transform->name == "Tail") {
			cattail = drawable.transform;
		}
		else {
			throw std::runtime_error("Found unexpected object: " + drawable.transform->name);
		}
	}
	catTransforms = { cat, lbpeet, lfpeet, rbpeet, rfpeet, lear, rear, cattail };
	if (grass == nullptr) throw std::runtime_error("Grass object not found.");
	
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//spawn the tunnel
	srand((unsigned)time(NULL));
	for (uint32_t i = 0; i < len_tiles; ++i) {
		for (uint32_t j = 0; j < num_tiles; ++j) {
			float randnum = (float)rand() / RAND_MAX;
			
			if (randnum < 0.5f) {
				//std::cout << randnum << "\n";
				float angle  = 360.f * (float)j / num_tiles;
				float depth = (-5.0f * (float)len_tiles) * i;
				new_block(angle, depth);
			}
		}
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a || evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d || evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a || evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d || evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			up.pressed = false;
			return true;
		}
	}
	return false;
}

void tunnel_update(float elapsed) {

}

void PlayMode::update(float elapsed) {
	//rotate tunnel
	if (left.pressed) {
		//iterate through tiles and rotate transforms by rotation_speed * elapsed
		rotation += rotation_speed * elapsed;
	}
	else if (right.pressed) {
		//same thing but other direction
		rotation -= rotation_speed * elapsed;
	}

	//check if grounded aka collision with tile

	if (up.pressed && grounded) {
		cat_speed = 5.0f; //sorry this is just a magic number rn
		grounded = false; //perhaps we don't need this
	}
	if (grounded) {
		cat_speed = 0.0f;
	}
	else {
		cat_speed += gravity * elapsed;
	}
	//update cat position
	for (Scene::Transform* t : catTransforms) {
		t->position.z += (cat_speed * elapsed);
	}

	//keeping this code for reference on animation
	/*
	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 200.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 200.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 300.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	*/

	for(Block &block : blocks) {
		block.depth += elapsed * block_speed;
		block.update_pos(rotation);
	}

	block_speed += elapsed;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Arrows/A+D to rotate tunnel, Space to jump",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Arrows/A+D to rotate tunnel, Space to jump",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
