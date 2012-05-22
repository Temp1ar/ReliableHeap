#include <iostream>
#include <cstdlib>
#include <list>
#include <cassert>
#include <functional>

template<class T, class Comparator = std::less<T>>
class Heap 
{
	class Node 
	{
	public:
		Node(T value, Node* parent = 0, Node* left = 0, Node* right = 0) 
			: value_(value)
			, parent_(parent)
			, left_(left)
			, right_(right)
		{}

		Node(const Node& other) 
			: value_(other.value_)
			, parent_(other.parent_)
			, left_(other.left_)
			, right_(other.right_)
		{}

		void swapWithChild(Node* other) 
		{
			// Partying hard with pointers...
			assert(other != 0);

			Node* rootParent = parent_;
			Node* rootLeft = left_;
			Node* rootRight = right_;

			parent_ = other;

			if (right_ == other) {
				if (left_) {
					left_->parent_ = other;
				}
				left_ = other->left_;
				right_ = other->right_;
				other->left_ = rootLeft;
				other->right_ = this;
			} else if (left_ == other) {
				if (right_) {
					right_->parent_ = other;
				}
				left_ = other->left_;
				right_ = other->right_;
				other->left_ = this;
				other->right_ = rootRight;
			} else {
				return;
			}

			other->parent_ = rootParent;

			if (rootParent) {
				if (rootParent->left_ == this) {
					rootParent->left_ = other;
				} else {
					rootParent->right_ = other;
				}
			}

			if (left_) {
				left_->parent_ = this;
			}

			if (right_) {
				right_->parent_ = this;
			}
		}

		T value_;
		Node* left_;
		Node* right_;
		Node* parent_;
	};

	public:
	Heap() 
		: root_(0)
		, bottomRight_(0)
		, size_(0)
	{}

	Heap(const Heap& h) 
		: root_(0)
		, bottomRight_(0)
		, size_(0)
	{
		if (h.size_ == 0) {
			return;
		}

		deepCopy(root_, h.root_, 0, h.bottomRight_);
	}

	Heap& operator=(const Heap& h) 
	{
		if (this != &h) {
			Heap(h).swap(*this);
		} 
		return *this;
	}

	~Heap() {
		while(size_ != 0) {
			swapAndDeleteLast();
		}
	}

	size_t size() {
		return size_;
	}

	void swap(Heap& h) 
	{
		std::swap(root_, h.root_);
		std::swap(bottomRight_, h.bottomRight_);
		std::swap(size_, h.size_);
	}

	void push(T const& t) 
	{
		Node* newNode = new Node(t);

		if (root_ == 0) {
			root_ = bottomRight_ = newNode;
			++size_;
		} else {
			newNode->parent_ = bottomRight_;
			if (bottomRight_->left_ == 0) {
				bottomRight_->left_  = newNode;
				++size_;
				try {
					// sieve can throw comparator exceptions
					sieve(bottomRight_);
				} catch(const std::exception&) {
					// no swaps yet, rolling back
					bottomRight_->left_ = 0;
					--size_;
					delete newNode;
				}
			} else {
				bottomRight_->right_ = newNode;
				++size_;
				try {
					// sieve can throw comparator exceptions
					sieve(bottomRight_);
					bottomRight_ = findBottomRight();
				} catch(const std::exception&) {
					// no swaps yet, rolling back
					bottomRight_->right_ = 0;
					--size_;
					delete newNode;
				}				
			}
		}
	}

	const T& top() const
	{
		return root_->value_;
	}

	void pop() 
	{
		Node* rootBackup = new Node(*root_);
		Node* lastParent;
		Node* last = getLast();
		if(last->parent_) {
			lastParent = last->parent_;	
		}
		swapAndDeleteLast();
		if(root_ == 0) {
			return;
		}

		try {
			sieve(root_);
			delete rootBackup;
		} catch(const std::exception&) {
			//bottomRight_ = bottomRightBackup;
			last = root_;
			if (lastParent->left_ == 0) {
				lastParent->left_ = last;
			} else {
				lastParent->right_ = last;
			}
			last->left_  = 0;
			last->right_ = 0;
			last->parent_ = lastParent;
			root_ = rootBackup;
			root_->left_->parent_ = root_;
			root_->right_->parent_= root_;
			++size_;
		}
	}

private:
	Node* root_;
	Node* bottomRight_;
	size_t size_;

	void swapAndDeleteLast() 
	{
		if (size_ == 1) {
			delete root_;
			root_ = 0;
			bottomRight_ = 0;
			--size_;
			return;
		}

		Node* last = getLast();
		assert(last->right_ == 0);
		assert(last->left_ == 0);

		if (last->parent_->right_ == last) {
			last->parent_->right_ = 0;
		} else {
			last->parent_->left_ = 0;
		}

		if (root_->left_) 
			root_->left_->parent_ = last;
		if (root_->right_)
			root_->right_->parent_ = last;

		last->left_  = root_->left_;
		last->right_ = root_->right_;
		last->parent_ = 0;

		delete root_;
		root_ = last;
		--size_;

		if (bottomRight_ == last) {
			bottomRight_ = getLast()->parent_;
		}
	}
	
	void sieve(Node* root) 
	{
		Node* minimum = root;
		if (root->left_) {
			minimum = root->left_;
		}

		Comparator comp = Comparator();
		if (root->right_) {
			if (comp(root->right_->value_, minimum->value_)) {
				minimum = root->right_;
			}
		}

		//if(minimum->value_ == 8 && root->value_ == 14)
		//	throw std::exception();

		if (comp(minimum->value_, root->value_)) {
			swapNodes(root, minimum);

			try {
				if (minimum->parent_) {
					sieve(minimum->parent_);
				}
				if (minimum->left_) {
					sieve(minimum->left_);
				}
				if (minimum->right_) {
					sieve(minimum->right_);
				}
			} catch(const std::exception& e) {
				swapNodes(minimum, root);
				throw e;
			}
		}
	}

	void swapNodes( Node* root, Node* minimum ) 
	{
		root->swapWithChild(minimum);
		if (root == root_) {
			root_ = minimum;
		}
		if (root == bottomRight_) {
			bottomRight_ = minimum;
		} else if (minimum == bottomRight_) {
			bottomRight_ = root;
		}
	}

	Node* getLast() const
	{
		size_t value = size_;
		std::list<char> bits;
		while (value != 0) {
			bits.push_front(value % 2);
			value /= 2;
		}
		
		Node* answer = root_;
		std::list<char>::iterator it = ++bits.begin();
		for (; it != bits.end(); ++it) {
			if (*it) {
				answer = answer->right_;
			} else {
				answer = answer->left_;
			}
		}
		return answer;
	}

	Node* findBottomRight() const
	{
		Node* root = bottomRight_;

		// if size_+1 is power of two
		if (((size_+1) & ((size_ + 1) - 1)) == 0) {
			root = root_;
			while (root->left_ != 0) {
				root = root->left_;
			}
		} else {
			while (root->parent_->right_ == root) {
				root = root->parent_;
			}
			root = root->parent_;
			root = root->right_;
			while (root->left_) {
				root = root->left_;
			}
		}
		return root;
	}

	void deepCopy(Node*& p, Node* h, Node* parent, Node* bottomRight)
	{
		p = new Node(h->value_, parent);
		if (bottomRight == h) {
			bottomRight_ = p;
		}
		++size_;

		if (h->left_) {
			deepCopy(p->left_, h->left_, p, bottomRight);
		}

		if (h->right_) {
			deepCopy(p->right_, h->right_, p, bottomRight);
		}
	}
};


int main () 
{
	//_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(0));

	Heap<int> h;

	for (int i = 1; i < 15; ++i) {
		h.push(i);
	}

	h.pop();
	//h.pop();

	while (h.size() > 0) {
		std::cout << h.top() << " ";
		h.pop();
	}

	Heap<int> h2(h);
	Heap<int> h3;
	h3 = h2;

	while (h3.size() > 0) {
		std::cout << h3.top() << " ";
		h3.pop();
	}
	
	return 0;
}