speed = 0.5

function tick(dt)
	local obj = tracer.scene:getObject(0)
	obj.transform = obj.transform * tracer.Mat.fromTranslation(tracer.Vec(speed * dt, 0, 0))
end

