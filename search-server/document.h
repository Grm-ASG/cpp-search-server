#pragma once

struct Document {
	Document();
	Document(int id, double relevance, int rating);

	int		id;
	double	relevance;
	int		rating;
};