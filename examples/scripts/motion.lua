speed = 1
cameraSpeed = math.pi / 8
rot = math.pi / 2

light = tracer.SpotLight(tracer.Color(1, 1, 1), tracer.Vec(2, 5, 0), tracer.Vec(0, 0, 0), math.pi / 12, math.pi / 9)
tracer.scene:addLight(light)

function tick(dt)
	local obj = tracer.scene:getObject(3)
	obj.transform = obj.transform * tracer.Mat.fromRotationZ(dt * speed)
	obj.transform = tracer.Mat.fromTranslation(tracer.Vec(-dt * speed, 0, 0)) * obj.transform
	local m = tracer.Mat.fromTranslation(tracer.Vec(0, -1, 0)) * obj.transform
	local pos = m:mul(tracer.Vec(0, 0, 0))
	light.direction = pos - light.position
	
	local cam = tracer.scene.camera
	local mat = tracer.Mat.fromTranslation(tracer.Vec(0, 0, -6)) * tracer.Mat.fromRotationY(rot) * tracer.Mat.fromTranslation(tracer.Vec(0, 0, 8))
	cam.position = mat:mul(tracer.Vec(0, 0, 0))
	tracer.scene.camera = cam
	rot = rot - dt * cameraSpeed
end
