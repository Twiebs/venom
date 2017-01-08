
#if 0
	if (gl_FragCoord.z < u_shadow_cascade_distance[3])W
		out_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	if (gl_FragCoord.z < u_shadow_cascade_distance[2])
		out_color = vec4(0.0f, 1.0f, 1.0f, 1.0f);
	if (gl_FragCoord.z < u_shadow_cascade_distance[1])
		out_color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	if (gl_FragCoord.z < u_shadow_cascade_distance[0])
		out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
