class Node {
public:
    int data;
    Node* left;
    Node* right;

    Node(int val) {
        data = val;
        left = right = NULL;
    }
};

class BST {
private:
    Node* root;

    // Helper to insert
    Node* insert(Node* node, int val) {
        if (node == NULL)
            return new Node(val);

        if (val < node->data)
            node->left = insert(node->left, val);
        else
            node->right = insert(node->right, val);

        return node;
    }

    // Inorder traversal (sorted output)
    void inorder(Node* node) {
        if (node == NULL) return;

        inorder(node->left);
        printf("%d ", node->data);
        inorder(node->right);
    }

    // Search helper
    Node* search(Node* node, int key) {
        if (node == NULL || node->data == key)
            return node;

        if (key < node->data)
            return search(node->left, key);
        else
            return search(node->right, key);
    }

public:
    BST() {
        root = NULL;
    }

    void insert(int val) {
        root = insert(root, val);
    }

    void display() {
        printf("Inorder Traversal: ");
        inorder(root);
        printf("\n");
    }

    void find(int key) {
        Node* res = search(root, key);
        if (res != NULL)
            printf("Element %d found in BST\n", key);
        else
            printf("Element %d not found in BST\n", key);
    }
};

int main() {
    BST tree;
    int choice, val;

    while (1) {
        printf("\n1. Insert\n2. Display (Inorder)\n3. Search\n4. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter value to insert: ");
                scanf("%d", &val);
                tree.insert(val);
                break;

            case 2:
                tree.display();
                break;

            case 3:
                printf("Enter value to search: ");
                scanf("%d", &val);
                tree.find(val);
                break;

            case 4:
                return 0;

            default:
                printf("Invalid choice!\n");
        }
    }

    return 0;
}