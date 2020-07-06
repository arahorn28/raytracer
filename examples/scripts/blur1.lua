speed = 1

function tick(dt)
	local cam = tracer.scene.camera
	cam.position = cam.position + tracer.Vec(speed * dt, 0, 0)
	tracer.scene.camera = cam
end

