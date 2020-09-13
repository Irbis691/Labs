#include "Common_textures.h"

using namespace std;

// Этот файл сдаётся на проверку
// Здесь напишите реализацию необходимых классов-потомков `IShape`

class ShapeImpl : public IShape {
public:
	void SetPosition(Point point) override {
		point_ = point;
	}

	Point GetPosition() const override {
		return point_;
	}

	void SetSize(Size size) override {
		size_ = size;
	}

	Size GetSize() const override {
		return size_;
	}

	void SetTexture(std::shared_ptr<ITexture> ptr) override {
		texture_ = ptr;
	}

	ITexture* GetTexture() const override {
		return texture_.get();
	}

protected:
	Point point_;
	Size size_;
	shared_ptr<ITexture> texture_;
};

class Rectangle : public ShapeImpl {
public:
	unique_ptr<IShape> Clone() const override {
		auto ptr = make_unique<Rectangle>();
		ptr->SetPosition(this->point_);
		ptr->SetSize(this->size_);
		ptr->SetTexture(this->texture_);
		return ptr;
	}

	void Draw(Image& image) const override {
		for (size_t i = point_.y;
			 i < size_.height + point_.y && i < image.size();
			 ++i) {
			for (size_t j = point_.x;
				 j < size_.width + point_.x && j < image.begin()->size();
				 ++j) {
				if (texture_
					&& i - point_.y < texture_->GetSize().height
					&& j - point_.x < texture_->GetSize().width) {
					image[i][j] = texture_->GetImage()[i - point_.y][j - point_.x];
				} else {
					image[i][j] = '.';
				}
			}
		}
	}
};

class Circle : public ShapeImpl {
	unique_ptr<IShape> Clone() const override {
		auto ptr = make_unique<Circle>();
		ptr->SetPosition(this->point_);
		ptr->SetSize(this->size_);
		ptr->SetTexture(this->texture_);
		return ptr;
	}

	void Draw(Image& image) const override {
		for (int i = point_.y;
			 i < size_.height + point_.y && i < image.size();
			 ++i) {
			for (int j = point_.x;
				 j < size_.width + point_.x && j < image.begin()->size();
				 ++j) {
				if (IsPointInEllipse({j - point_.x, i - point_.y}, size_)) {
					if (texture_
						&& i - point_.y < texture_->GetSize().height
						&& j - point_.x < texture_->GetSize().width) {
						image[i][j] = texture_->GetImage()[i - point_.y][j - point_.x];
					} else {
						image[i][j] = '.';
					}
				}
			}
		}
	}
};

// Напишите реализацию функции
unique_ptr<IShape> MakeShape(ShapeType shape_type) {
	if (shape_type == ShapeType::Rectangle) {
		return make_unique<Rectangle>();
	}
	return make_unique<Circle>();
}