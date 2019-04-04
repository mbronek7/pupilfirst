class AddTablesForCommunity < ActiveRecord::Migration[5.2]
  def up
    create_table :channels do |t|
      t.string :name
      t.references :school

      t.timestamps
    end

    create_table :questions do |t|
      t.string :title
      t.text :description
      t.references :channel
      t.references :user
      t.references :targets

      t.timestamps
    end

    create_table :answers do |t|
      t.text :description
      t.references :question
      t.references :user

      t.timestamps
    end

    create_table :answer_claps do |t|
      t.integer :count
      t.references :answer
      t.references :user

      t.timestamps
    end
  end

  def down
    drop_table :channels
    drop_table :questions
    drop_table :answers
    drop_table :answer_claps
  end
end
