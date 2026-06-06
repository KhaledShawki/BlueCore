bb.log = bb.log or {}

function bb.log.info(message)
    print("[BlueBuild] " .. message)
end

function bb.log.warn(message)
    print("[BlueBuild warning] " .. message)
end
